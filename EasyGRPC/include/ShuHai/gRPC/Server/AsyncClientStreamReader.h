#pragma once

#include "ShuHai/gRPC/Server/TypeTraits.h"
#include "ShuHai/gRPC/AsyncAction.h"

#include <future>

namespace ShuHai::gRPC::Server
{
    template<typename RequestFunc>
    class AsyncClientStreamCallHandler;

    template<typename RequestFuncType>
    class AsyncClientStreamReader
    {
    public:
        SHUHAI_GRPC_SERVER_EXPAND_AsyncRequestTraits(RequestFuncType);

        [[nodiscard]] const Request& current() const { return _current; }

        std::future<bool> moveNext()
        {
            auto action = new ReadAction(this, _current);
            return action->result();
        }

    private:
        friend class AsyncClientStreamCallHandler<RequestFunc>;

        class ReadAction : public AsyncAction<bool>
        {
        public:
            explicit ReadAction(AsyncClientStreamReader* owner, Request& request)
                : _owner(owner)
            {
                _owner->_stream.Read(&request, this);
            }

            void finalizeResult(bool ok) override
            {
                setResult(ok);
                _owner->onFinalizeRead(this, ok);
            }

        private:
            AsyncClientStreamReader* const _owner;
        };

        AsyncClientStreamReader(
            Service* service, grpc::ServerCompletionQueue* cq, std::function<void(AsyncClientStreamReader*)> onFinished)
            : _stream(&_context)
            , _onFinished(std::move(onFinished))
        { }

        void onFinalizeRead(ReadAction* action, bool ok)
        {
            // ok indicates whether there is a valid message that got read.
            // If not, you know that there are certainly no more messages that can ever be read from this stream.
            // This could happen because the client has done a WritesDone already.

            if (ok)
            {
                // Next read should called by user.
            }
            else
            {
                _onFinished(this);
            }
        }

        grpc::ServerContext _context;
        StreamingInterface _stream;
        Request _current;

        std::function<void(AsyncClientStreamReader*)> _onFinished;
    };
}
