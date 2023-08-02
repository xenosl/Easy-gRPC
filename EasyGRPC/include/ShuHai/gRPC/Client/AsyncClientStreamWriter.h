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

        class StreamAction : public AsyncAction<bool>
        {
        public:
            explicit StreamAction(AsyncClientStreamWriter* owner)
                : _owner(owner)
            { }

            void perform()
            {
                std::lock_guard slg(_owner->_stateMutex);
                assert(_owner->_state == State::Ready);
                _owner->_state = State::Processing;
                performImpl();
            }

            void finalizeResult(bool ok) final
            {
                {
                    std::lock_guard slg(_owner->_stateMutex);
                    assert(_owner->_state == State::Processing);
                    setResult(ok);
                    _owner->_state = State::Ready;
                }
                finalizeResultImpl(ok);
            }

        protected:
            virtual void performImpl() = 0;

            virtual void finalizeResultImpl(bool ok) = 0;

            AsyncClientStreamWriter* const _owner;
        };

        class WriteAction : public StreamAction
        {
        public:
            WriteAction(AsyncClientStreamWriter* owner, Request message, grpc::WriteOptions options)
                : StreamAction(owner)
                , _message(std::move(message))
                , _options(options)
            { }

            [[nodiscard]] const Request& message() const { return _message; }

            [[nodiscard]] const grpc::WriteOptions& options() { return _options; }

        protected:
            void performImpl() override { this->_owner->_stream.Write(_message, _options, this); }

            void finalizeResultImpl(bool ok) override { this->_owner->finalizeWrite(this, ok); }

        private:
            Request _message;
            grpc::WriteOptions _options;
        };

        class FinishAction : public StreamAction
        {
        public:
            explicit FinishAction(AsyncClientStreamWriter* owner)
                : StreamAction(owner)
            { }

        protected:
            void performImpl() override { this->_owner->_stream.Finish(&this->_owner->_status, this); }

            void finalizeResultImpl(bool ok) override { this->_owner->finalizeFinish(this, ok); }
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

        bool tryPerformAction(StreamAction* action)
        {
            switch (_state)
            {
            case State::Ready:
                action->perform();
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

        void finalizeWrite(WriteAction* action, bool ok)
        {
            // ok means that the data/metadata/status/etc is going to go to the wire.
            // If it is false, it not going to the wire because the call is already dead (i.e., canceled,
            // deadline expired, other side dropped the channel, etc).

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
                                writeAction->perform();
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
                            finishAction->perform();
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

            _finishCallback();
        }

        template<typename T, typename... Args>
        std::future<bool> newAction(Args... args)
        {
            static_assert(std::is_base_of_v<StreamAction, T>);

            auto action = new T(std::forward<Args>(args)...);
            auto result = action->result();
            {
                std::lock_guard l(_actionsMutex);
                _actions.emplace(action);
            }
            return result;
        }

        StreamAction* popAction()
        {
            auto action = _actions.front();
            _actions.pop();
            return action;
        }

        std::mutex _stateMutex;
        State _state { State::Ready };

        std::mutex _actionsMutex;
        std::queue<StreamAction*> _actions;
    };
}
