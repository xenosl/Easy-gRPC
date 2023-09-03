#pragma once

#include "Task.grpc.pb.h"

#include <ShuHai/gRPC/Server/AsyncServer.h>

namespace ShuHai::gRPC::Examples
{
    class TaskRpcService
    {
    public:
        template<typename... Services>
        static void registerTo(Server::AsyncServer<Services...>& server)
        {
            using Service = Proto::Task::Rpc::AsyncService;
            server.registerCallHandler(&Service::RequestStartTask, &StartTask);
            server.registerCallHandler(&Service::RequestGetTarget, &GetTarget);
        }

        static Proto::Task::StartTaskReply StartTask(
            grpc::ServerContext& context, const Proto::Task::StartTaskRequest& request);
        static Proto::Task::GetTargetReply GetTarget(
            grpc::ServerContext& context, const Proto::Task::GetTargetRequest& request);
    };
}
