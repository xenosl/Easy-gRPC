#pragma once

#include "Generated/App.grpc.pb.h"

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

        void Launch(const Proto::App::LaunchRequest& request,
            std::function<void(const Proto::App::LaunchReply&)> onSucceed = nullptr,
            std::function<void(const grpc::Status&)> onFailed = nullptr)
        {
            _client.call(&Proto::App::Rpc::Stub::AsyncLaunch, request, std::move(onSucceed), std::move(onFailed));
        }

    private:
        AsyncClient& _client;
    };
}
