#pragma once

#include "ShuHai/gRPC/Client/TypeTraits.h"
#include "ShuHai/gRPC/IAsyncAction.h"
#include "ShuHai/gRPC/StreamingError.h"

#include <queue>
#include <future>
#include <mutex>
#include <type_traits>

namespace ShuHai::gRPC::Client
{
    namespace Internal
    {
        enum class AsyncClientStreamWriterState
        {
            Ready,
            Processing,
            Finished
        };
    }

    template<typename CallFunc>
    class AsyncClientStreamCall;

    template<typename CallFunc>
    class AsyncClientStreamWriter
    {
    public:
        SHUHAI_GRPC_CLIENT_EXPAND_AsyncCallTraits(CallFunc);

    private:
        friend class AsyncClientStreamCall<CallFunc>;

        AsyncClientStreamWriter(grpc::CompletionQueue* cq, StreamingInterface& stream, grpc::Status& status,
            std::function<void()> finishCallback)
            : _cq(cq)
            , _stream(stream)
            , _status(status)
            , _finishCallback(std::move(finishCallback))
        { }

        grpc::CompletionQueue* const _cq;
        StreamingInterface& _stream;
        grpc::Status& _status;
        std::function<void()> _finishCallback;

        // Actions -----------------------------------------------------------------------------------------------------
    public:
        /**
		 * \brief Append writing of the specified \p message with certain \p options as an async-action to a queue. The
         *  writing is performed as soon as it is allowed by the completion queue.
		 * \param message The message to be written.
		 * \param options The options to be used to write this message.
		 * \return The future object that identifies whether the message is going to the wire or any exceptions occurs.
		 */
        std::future<bool> write(Request message, grpc::WriteOptions options = {})
        {
            auto okFuture = newAction<WriteAction>(this, std::move(message), options);
            tryPerformAction();
            return okFuture;
        }

        std::future<bool> finish()
        {
            auto okFuture = newAction<FinishAction>(this);
            tryPerformAction();
            return okFuture;
        }

    private:
        using State = Internal::AsyncClientStreamWriterState;

        class AsyncAction : public IAsyncAction
        {
        public:
            explicit AsyncAction(AsyncClientStreamWriter* owner)
                : _owner(owner)
            { }

            virtual void perform() = 0;

            std::future<bool> getOkFuture() { return _okPromise.get_future(); }

            void setResult(bool ok) { _okPromise.set_value(ok); }

            template<typename T, typename... Args>
            void setException(Args... args)
            {
                try
                {
                    throw T(std::forward<Args>(args)...);
                }
                catch (...)
                {
                    _okPromise.set_exception(std::current_exception());
                }
            }

        protected:
            AsyncClientStreamWriter* const _owner;
            std::promise<bool> _okPromise;
        };

        class WriteAction : public AsyncAction
        {
        public:
            WriteAction(AsyncClientStreamWriter* owner, Request message, grpc::WriteOptions options)
                : AsyncAction(owner)
                , _message(std::move(message))
                , _options(options)
            { }

            void perform() override { this->_owner->_stream.Write(_message, _options, this); }

            void finalizeResult(bool ok) override { this->_owner->finalizeWrite(this, ok); }

            [[nodiscard]] const Request& message() const { return _message; }

            [[nodiscard]] const grpc::WriteOptions& options() { return _options; }

        private:
            Request _message;
            grpc::WriteOptions _options;
        };

        class FinishAction : public AsyncAction
        {
        public:
            explicit FinishAction(AsyncClientStreamWriter* owner)
                : AsyncAction(owner)
            { }

            void perform() override { this->_owner->_stream.Finish(&this->_owner->_status, this); }

            void finalizeResult(bool ok) override { this->_owner->finalizeFinish(this, ok); }
        };

        bool tryPerformAction()
        {
            // Ensure action pop and perform in the whole is exclusive because:
            // 1. Only one action is allowed to perform at the same time.
            // 2. Performing order of actions have to keep the same as it pushes.
            std::lock_guard alg(_actionsMutex);

            if (_actions.empty())
                return false;

            auto action = popAction();
            return tryPerformAction(action);
        }

        template<typename T>
        bool tryPerformAction(T* action)
        {
            static_assert(std::is_base_of_v<AsyncAction, T>);

            switch (_state)
            {
            case State::Ready:
                performAction(action);
                return true;
            case State::Finished:
                action->template setException<InvalidStreamingAction>(
                    "The stream is finished, no more action is allowed.");
                return false;
            case State::Processing:
                // Process in finalizeXxx function.
                return false;
            default:
                assert("Invalid state.");
                return false;
            }
        }

        template<typename T>
        void performAction(T* action)
        {
            static_assert(std::is_base_of_v<AsyncAction, T>);

            std::lock_guard slg(_stateMutex);

            action->perform(); // No vtable searching when T is a derived type.
            _state = State::Processing;
        }

        void finalizeWrite(WriteAction* action, bool ok)
        {
            // ok means that the data/metadata/status/etc is going to go to the wire.
            // If it is false, it not going to the wire because the call is already dead (i.e., canceled,
            // deadline expired, other side dropped the channel, etc).
            {
                std::lock_guard slg(_stateMutex);

                assert(_state == State::Processing);

                action->setResult(ok);
                _state = State::Ready;
            }

            if (ok)
            {
                std::lock_guard alg(_actionsMutex);

                bool lastMessageWritten = action->options().is_last_message();
                bool finishPerformed = false;
                while (!_actions.empty())
                {
                    auto nextAction = popAction();
                    if (auto writeAction = dynamic_cast<WriteAction*>(nextAction))
                    {
                        if (lastMessageWritten)
                        {
                            writeAction->template setException<InvalidStreamingAction>(
                                "Last already written, no more message is allowed.");
                        }
                        else
                        {
                            if (finishPerformed)
                            {
                                writeAction->template setException<InvalidStreamingAction>(
                                    "Attempt to write after finish.");
                            }
                            else
                            {
                                performAction(writeAction);
                            }
                        }
                    }
                    else if (auto finishAction = dynamic_cast<FinishAction*>(nextAction))
                    {
                        if (finishPerformed)
                        {
                            finishAction->template setException<InvalidStreamingAction>("Duplicate finish call.");
                        }
                        else
                        {
                            performAction(finishAction);
                            finishPerformed = true;
                        }
                    }
                }
            }
            else
            {
                finish();
            }
        }

        void finalizeFinish(FinishAction* action, bool ok)
        {
            // ok should always be true
            assert(ok);

            action->setResult(ok);

            _finishCallback();
        }

        template<typename T, typename... Args>
        std::future<bool> newAction(Args... args)
        {
            static_assert(std::is_base_of_v<AsyncAction, T>);

            auto action = new T(std::forward<Args>(args)...);
            auto okFuture = action->getOkFuture();
            {
                std::lock_guard l(_actionsMutex);
                _actions.emplace(action);
            }
            return okFuture;
        }

        AsyncAction* popAction()
        {
            auto action = _actions.front();
            _actions.pop();
            return action;
        }

        std::mutex _stateMutex;
        State _state { State::Ready };

        std::mutex _actionsMutex;
        std::queue<AsyncAction*> _actions;
    };
}
