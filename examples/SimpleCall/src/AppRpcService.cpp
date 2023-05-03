#include "AppRpcService.h"
#include "ShuHai/gRPC/Examples/Console.h"

namespace ShuHai::gRPC::Examples
{
    void AppRpcService::Launch(
        grpc::ServerContext& context, const Proto::App::LaunchRequest& request, Proto::App::LaunchReply& reply)
    {
        reply.set_succeed(true);

        console().writeLine("AppRpcService::Launch");
    }

    void AppRpcService::Shutdown(
        grpc::ServerContext& context, const Proto::App::ShutdownRequest& request, Proto::App::ShutdownReply& reply)
    {
        reply.set_succeed(true);

        console().writeLine("AppRpcService::Shutdown");
    }
}
