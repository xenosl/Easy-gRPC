#include "Generated/Streaming.grpc.pb.h"

#include "ShuHai/gRPC/Examples/Console.h"
#include "ShuHai/gRPC/Examples/Thread.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>
#include <ShuHai/gRPC/Client/AsyncClient.h>

using namespace ShuHai::gRPC;
using namespace ShuHai::gRPC::Examples;
using namespace ShuHai::gRPC::Examples::Streaming;

using AsyncServer = Server::AsyncServer<ProcessUtil::AsyncService>;
using AsyncClient = Client::AsyncClient<ProcessUtil::Stub>;

void registerServerStreamHandler(AsyncServer& server)
{
    auto makeReply = [](const GetProcessesRequest& request, int n)
    {
        ProcessInfo info;
        info.set_pid(n);
        info.set_name("Process " + std::to_string(n));
        return info;
    };

    server.registerCallHandler(&ProcessUtil::AsyncService::RequestGetProcesses,
        [makeReply = std::move(makeReply)](
            grpc::ServerContext& context, const GetProcessesRequest& request, auto& streamWriter)
        {
            streamWriter.write(makeReply(request, 1));
            streamWriter.write(makeReply(request, 2));
            streamWriter.write(makeReply(request, 3));
            streamWriter.write(makeReply(request, 4));
            streamWriter.finish();
        });
}

void serverStream(AsyncClient& client)
{
    GetProcessesRequest request;
    request.set_filter("*");

    // Call and get response by callback
    auto callForCallback = client.call(&ProcessUtil::Stub::AsyncGetProcesses, request);
    auto stream1 = callForCallback->responseStream().get();
    std::function<void(decltype(stream1))> nextMoved;
    nextMoved = [&](auto s)
    {
        const auto& info = s->current();
        console().writeLine("[%s] %d - %s", "ServerStream-Callback", info.pid(), info.name().c_str());
        if (!s->finished())
            s->moveNext(nextMoved);
    };
    stream1->moveNext(nextMoved);

    // Call and wait for the responses
    auto callForWait = client.call(&ProcessUtil::Stub::AsyncGetProcesses, request);
    auto stream2 = callForWait->responseStream().get();
    while (stream2->moveNext().get())
    {
        const auto& info = stream2->current();
        console().writeLine("[%s] %d - %s", "ServerStream-Wait", info.pid(), info.name().c_str());
    }
}

void registerClientStreamHandler(AsyncServer& server)
{
    server.registerCallHandler(&ProcessUtil::AsyncService::RequestPostProcesses,
        [](grpc::ServerContext& context, auto& streamReader)
        {
            while (streamReader.moveNext().get())
            {
                const ProcessInfo& request = streamReader.current();
                console().writeLine(
                    "[ClientStream-Read] PostProcesses: %d - %s", request.pid(), request.name().c_str());
            }

            PostProcessesReply reply;
            reply.set_ok(true);
            return reply;
        });
}

void clientStream(AsyncClient& client)
{
    auto call = client.call(&ProcessUtil::Stub::AsyncPostProcesses);
    auto stream = call->requestStream().get();
    for (int i = 0; i < 10; ++i)
    {
        ProcessInfo request;
        request.set_pid(i);
        request.set_name(std::to_string(i));
        auto written = stream->write(request).get();
        if (!written)
            break;
        console().writeLine("[ClientStream-Written] GetProcessesRequest: %s", request.name().c_str());
    }
    stream->finish().wait();
    console().writeLine("[ClientStream-Finish] GetProcessesRequest");
}

int main(int argc, char* argv[])
{
    constexpr uint16_t Port = 55212;

    // Build server and start the server.
    AsyncServer server(Port);
    registerServerStreamHandler(server);
    registerClientStreamHandler(server);
    server.start();

    // Wait for the server start.
    waitFor(100);

    // Build client.
    AsyncClient client("localhost:" + std::to_string(Port));
    serverStream(client);
    clientStream(client);

    // Wait for all calls done.
    waitFor(100);
    return EXIT_SUCCESS;
}
