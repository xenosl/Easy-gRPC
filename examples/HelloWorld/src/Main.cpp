#include "Generated/helloworld.grpc.pb.h"

#include "ShuHai/gRPC/Examples/Console.h"
#include "ShuHai/gRPC/Examples/Thread.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>
#include <ShuHai/gRPC/Client/AsyncClient.h>

using namespace ShuHai::gRPC;
using namespace ShuHai::gRPC::Examples;
using namespace HelloWorld;

using AsyncServer = Server::AsyncServer<Greeter::AsyncService>;
using AsyncClient = Client::AsyncClient<Greeter::Stub>;

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

void registerUnaryCallHandler(AsyncServer& server)
{
    server.registerCallHandler(&Greeter::AsyncService::RequestSayHello,
        [](grpc::ServerContext& context, const HelloRequest& request, HelloReply& reply)
        {
            // Logic and data behind the server's behavior.
            std::string prefix("Hello ");
            reply.set_message(prefix + request.name());
        });
}

void registerServerStreamHandler(AsyncServer& server)
{
    auto makeReply = [](const HelloRequest& request, int n)
    {
        HelloReply reply;
        reply.set_message("Hello " + request.name() + " - " + std::to_string(n));
        return std::move(reply);
    };

    server.registerCallHandler(&Greeter::AsyncService::RequestSayHelloServerStream,
        [makeReply = std::move(makeReply)](
            grpc::ServerContext& context, const HelloRequest& request, auto& streamWriter)
        {
            // Logic and data behind the server's behavior.
            std::string prefix("Hello ");
            streamWriter.write(makeReply(request, 1));
            streamWriter.write(makeReply(request, 2));
            streamWriter.write(makeReply(request, 3));
            streamWriter.write(makeReply(request, 4));
            streamWriter.finish();
        });
}

void unaryCall(AsyncClient& client)
{
    HelloRequest request;
    request.set_name("user");

    // Call and get response by callback.
    client.call(
        &Greeter::Stub::AsyncSayHello, request, [](std::future<HelloReply>&& f) { handleResult(f, "Unary-Callback"); });

    // Call and wait for the response
    auto replyFuture = client.call(&Greeter::Stub::AsyncSayHello, request);
    handleResult(replyFuture, "Unary-Wait");
}

void serverStream(AsyncClient& client)
{
    HelloRequest request;
    request.set_name("user");

    // Call and wait for the responses
    auto callForWait = client.call(&Greeter::Stub::AsyncSayHelloServerStream, request);
    auto it2 = callForWait->responseIterator().get();
    while (it2->moveNext().get())
    {
        const auto& response = it2->current();
        console().writeLine("[%s] SayHello reply: %s", "ServerStream-Wait", response.message().c_str());
    }
}

int main(int argc, char* argv[])
{
    constexpr uint16_t Port = 55212;

    // Build server and start the server.
    AsyncServer server(Port);
    registerUnaryCallHandler(server);
    registerServerStreamHandler(server);
    server.start();

    // Wait for the server start.
    waitFor(100);

    // Build client.
    AsyncClient client("localhost:" + std::to_string(Port));
    unaryCall(client);
    serverStream(client);

    // Wait for all calls done.
    waitFor(100);
    return EXIT_SUCCESS;
}
