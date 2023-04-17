#include "Generated/helloworld.grpc.pb.h"

#include "ShuHai/gRPC/Examples/Console.h"
#include "ShuHai/gRPC/Examples/Thread.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>
#include <ShuHai/gRPC/Client/AsyncClient.h>

using namespace ShuHai::gRPC;
using namespace ShuHai::gRPC::Examples;
using namespace helloworld;

void handleResult(std::future<HelloReply>& f, const char* mode)
{
    try
    {
        auto reply = f.get();
        console().writeLine("[%s] SayHello reply: %s", mode, reply.message().c_str());
    }
    catch (const Client::AsyncCallError& e)
    {
        console().writeLine("[%s] Call failed(%d): %s", mode, e.status().error_code(), e.what());
    }
    catch (const std::exception& e)
    {
        console().writeLine("[%s] Call failed: %s", mode, e.what());
    }
}

int main(int argc, char* argv[])
{
    constexpr uint16_t Port = 55212;

    // Build server and start the server.
    Server::AsyncServer<Greeter::AsyncService> server(Port);
    server.registerCallHandler(&Greeter::AsyncService::RequestSayHello,
        [](const HelloRequest& request, HelloReply& reply)
        {
            // Logic and data behind the server's behavior.
            std::string prefix("Hello ");
            reply.set_message(prefix + request.name());
        });
    server.start();

    // Wait for the server start.
    waitFor(100);

    // Build client.
    Client::AsyncClient<Greeter::Stub> client("localhost:" + std::to_string(Port));

    HelloRequest request;
    request.set_name("user");

    // Call and get response by callback.
    client.call(&Greeter::Stub::PrepareAsyncSayHello, request,
        [](std::future<HelloReply>&& f) { handleResult(f, "Callback"); });

    // Call and wait for the response
    auto replyFuture = client.call(&Greeter::Stub::PrepareAsyncSayHello, request);
    handleResult(replyFuture, "Wait");

    // Wait for all calls done.
    waitFor(100);
    return EXIT_SUCCESS;
}
