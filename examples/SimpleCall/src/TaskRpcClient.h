#pragma once

#include "Generated/Task.grpc.pb.h"

#include <ShuHai/gRPC/Client/AsyncClient.h>

namespace ShuHai::gRPC::Examples
{
    template<typename... Stubs>
    class TaskRpcClient
    {
    public:
        using AsyncClient = Client::AsyncClient<Stubs...>;

        explicit TaskRpcClient(AsyncClient& client) : _client(client) { }

        auto GetTarget(const Proto::Task::GetTargetRequest& request)
        {
            return _client.call(&Proto::Task::Rpc::Stub::PrepareAsyncGetTarget, request);
        }

        void GetTarget(const Proto::Task::GetTargetRequest& request,
            std::function<void(std::future<Proto::Task::GetTargetReply>&&)> callback)
        {
            _client.call(&Proto::Task::Rpc::Stub::PrepareAsyncGetTarget, request, std::move(callback));
        }

    private:
        AsyncClient& _client;
    };
}
