#pragma once

#include "App.grpc.pb.h"

#include <ShuHai/gRPC/Client/AsyncClient.h>

namespace ShuHai::gRPC::Examples
{
    template<typename... Stubs>
    class AppRpcClient
    {
    public:
        using AsyncClient = Client::AsyncClient<Stubs...>;

        explicit AppRpcClient(AsyncClient& client)
            : _client(client)
        { }

        auto Launch(const Proto::App::LaunchRequest& request,
            std::function<void(std::shared_future<Proto::App::LaunchReply>)> callback)
        {
            return _client.call(&Proto::App::Rpc::Stub::AsyncLaunch, request, std::move(callback))->response();
        }

    private:
        AsyncClient& _client;
    };
}
