#pragma once

#include "Generated/Task.grpc.pb.h"

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

        static void StartTask(const Proto::Task::StartTaskRequest& request, Proto::Task::StartTaskReply& reply);
        static void GetTarget(const Proto::Task::GetTargetRequest& request, Proto::Task::GetTargetReply& reply);
    };
}
