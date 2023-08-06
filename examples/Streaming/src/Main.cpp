#include "Generated/Streaming.grpc.pb.h"

#include "ShuHai/gRPC/Examples/Console.h"
#include "ShuHai/gRPC/Examples/Thread.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>
#include <ShuHai/gRPC/Client/AsyncClient.h>

using namespace ShuHai::gRPC;
using namespace ShuHai::gRPC::Examples;
using namespace ShuHai::gRPC::Examples::Streaming;

using AsyncServer = Server::AsyncServer<Application::AsyncService>;
using AsyncClient = Client::AsyncClient<Application::Stub>;

using RequestFunc = decltype(&Application::AsyncService::RequestSetFeatures);

SetFeaturesReply handleClientStream(
    grpc::ServerContext& context, Server::AsyncClientStreamReader<RequestFunc>& streamReader)
{
    SetFeaturesReply reply;
    return reply;
}

void registerClientStreamHandler(AsyncServer& server)
{
    server.registerCallHandler(&Application::AsyncService::RequestSetFeatures, &handleClientStream);
}

void clientStream(AsyncClient& client)
{
    auto call = client.call(&Application::Stub::AsyncSetFeatures);
    auto& stream = call->streamWriter().get();
    for (int i = 0; i < 100; ++i)
    {
        Feature feature;
        feature.set_name(std::to_string(i));
        feature.set_value(std::to_string(i));
        stream->write(feature);
    }
    stream->finish();

    //auto response = call->response().get();
}

int main(int argc, char* argv[])
{
    constexpr uint16_t Port = 55212;
    AsyncServer server(Port);
    registerClientStreamHandler(server);
    server.start();

    // Wait for the server start.
    waitFor(100);

    // Build client.
    AsyncClient client("localhost:" + std::to_string(Port));
    clientStream(client);

    // Wait for all calls done.
    waitFor(100);
    return EXIT_SUCCESS;
}
