#include "AppRpcService.h"
#include "TaskRpcService.h"
#include "AppRpcClient.h"
#include "TaskRpcClient.h"
#include "ShuHai/gRPC/Examples/Console.h"
#include "ShuHai/gRPC/Examples/Thread.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>
#include <ShuHai/gRPC/Client/AsyncClient.h>

#include <thread>
#include <cstdlib>


using namespace ShuHai::gRPC::Examples;
using namespace ShuHai::gRPC;

using RpcServer = Server::AsyncServer<Proto::App::Rpc::AsyncService, Proto::Task::Rpc::AsyncService>;
using RpcClient = Client::AsyncClient<Proto::App::Rpc::Stub, Proto::Task::Rpc::Stub>;

template<typename... Stubs>
void demoGetTarget(TaskRpcClient<Stubs...>& client)
{
    std::vector<Proto::Task::GetTargetRequest> requests;
    for (int i = 0; i < 100; ++i)
    {
        Proto::Task::GetTargetRequest request;
        request.set_id(i + 1);
        requests.emplace_back(std::move(request));
    }

    // Get result by callback
    for (const auto& request : requests)
    {
        client.GetTarget(request,
            [](std::future<Proto::Task::GetTargetReply>&& f)
            {
                try
                {
                    auto reply = f.get();
                    console().writeLine("[Callback] GetTargetReply: %d", reply.target().id());
                }
                catch (const Client::AsyncCallError& e)
                {
                    console().writeLine("[Callback] Call failed(%d): %d", e.status().error_code(), e.what());
                }
                catch (const std::exception& e)
                {
                    console().writeLine("[Callback] Call failed: %s", e.what());
                }
            });
    }

    // Wait for the result
    for (const auto& request : requests)
    {
        auto result = client.GetTarget(request);
        try
        {
            const auto& reply = result.get();
            console().writeLine("[Future] GetTargetReply: %d", reply.target().id());
        }
        catch (const Client::AsyncCallError& e)
        {
            console().writeLine("[Future] GetTargetReply Error: %s", e.what());
        }
    }
}

int main(int argc, char* argv[])
{
    constexpr uint16_t Port = 55212;

    // Server
    RpcServer server(Port);
    AppRpcService::registerTo(server);
    TaskRpcService::registerTo(server);
    server.start();

    waitFor(10);

    // Client
    RpcClient client("localhost:" + std::to_string(Port));
    AppRpcClient appClient(client);
    TaskRpcClient taskClient(client);
    demoGetTarget(taskClient);

    waitFor(10);
    return EXIT_SUCCESS;
}
