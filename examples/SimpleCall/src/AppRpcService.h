#pragma once

#include "App.grpc.pb.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>

namespace ShuHai::gRPC::Examples
{
    class AppRpcService
    {
    public:
        template<typename... Services>
        static void registerTo(Server::AsyncServer<Services...>& server)
        {
            using Service = Proto::App::Rpc::AsyncService;
            server.registerCallHandler(&Service::RequestLaunch, &Launch);
            server.registerCallHandler(&Service::RequestShutdown, &Shutdown);
        }

        static Proto::App::LaunchReply Launch(grpc::ServerContext& context, const Proto::App::LaunchRequest& request);
        static Proto::App::ShutdownReply Shutdown(
            grpc::ServerContext& context, const Proto::App::ShutdownRequest& request);
    };
}
