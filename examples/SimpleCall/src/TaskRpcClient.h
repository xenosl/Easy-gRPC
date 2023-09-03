#pragma once

#include "Task.grpc.pb.h"

#include <ShuHai/gRPC/Client/AsyncClient.h>

namespace ShuHai::gRPC::Examples
{
    template<typename... Stubs>
    class TaskRpcClient
    {
    public:
        using AsyncClient = Client::AsyncClient<Stubs...>;

        explicit TaskRpcClient(AsyncClient& client)
            : _client(client)
        { }

        auto GetTarget(const Proto::Task::GetTargetRequest& request,
            std::function<void(std::shared_future<Proto::Task::GetTargetReply>)> callback)
        {
            return _client.call(&Proto::Task::Rpc::Stub::AsyncGetTarget, request, std::move(callback))->response();
        }

    private:
        AsyncClient& _client;
    };
}
