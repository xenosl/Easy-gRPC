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
        return reply;
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

void registerClientStreamHandler(AsyncServer& server)
{
    server.registerCallHandler(&Greeter::AsyncService::RequestSayHelloClientStream,
        [](grpc::ServerContext& context, auto& streamReader)
        {
            std::string replyMessage;
            while (streamReader.moveNext().get())
            {
                const HelloRequest& request = streamReader.current();
                replyMessage += "user " + request.name();
                replyMessage += '\n';
                console().writeLine("[ClientStream-Read] HelloRequest: %s", replyMessage.c_str());
            }
            replyMessage.pop_back();

            HelloReply reply;
            reply.set_message(std::move(replyMessage));
            console().writeLine("[ClientStream-Read] HelloReply: %s", reply.message().c_str());
            return reply;
        });
}

void unaryCall(AsyncClient& client)
{
    HelloRequest request;
    request.set_name("user");

    // Call and get response by callback
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

    // Call and get response by callback
    auto callForCallback = client.call(&Greeter::Stub::AsyncSayHelloServerStream, request);
    auto stream1 = callForCallback->responseStream().get();
    std::function<void(decltype(stream1))> nextMoved;
    nextMoved = [&](auto s)
    {
        const auto& response = s->current();
        console().writeLine("[%s] SayHello reply: %s", "ServerStream-Callback", response.message().c_str());
        if (!s->finished())
            s->moveNext(nextMoved);
    };
    stream1->moveNext(nextMoved);

    // Call and wait for the responses
    auto callForWait = client.call(&Greeter::Stub::AsyncSayHelloServerStream, request);
    auto stream2 = callForWait->responseStream().get();
    while (stream2->moveNext().get())
    {
        const auto& response = stream2->current();
        console().writeLine("[%s] SayHello reply: %s", "ServerStream-Wait", response.message().c_str());
    }
}

void clientStream(AsyncClient& client)
{
    auto call = client.call(&Greeter::Stub::AsyncSayHelloClientStream);
    auto stream = call->requestStream().get();
    for (int i = 0; i < 10; ++i)
    {
        HelloRequest request;
        request.set_name("user " + std::to_string(i));
        auto written = stream->write(request).get();
        if (!written)
            break;
        console().writeLine("[ClientStream-Written] HelloRequest: %s", request.name().c_str());
    }
    stream->finish().wait();
}

int main(int argc, char* argv[])
{
    constexpr uint16_t Port = 55212;

    // Build server and start the server.
    AsyncServer server(Port);
    registerUnaryCallHandler(server);
    registerServerStreamHandler(server);
    registerClientStreamHandler(server);
    server.start();

    // Wait for the server start.
    waitFor(100);

    // Build client.
    AsyncClient client("localhost:" + std::to_string(Port));
    unaryCall(client);
    serverStream(client);
    clientStream(client);

    // Wait for all calls done.
    waitFor(100);
    return EXIT_SUCCESS;
}
