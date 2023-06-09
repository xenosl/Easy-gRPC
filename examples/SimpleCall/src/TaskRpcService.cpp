#include "TaskRpcService.h"
#include "ShuHai/gRPC/Examples/Console.h"

namespace ShuHai::gRPC::Examples
{
    Proto::Task::StartTaskReply TaskRpcService::StartTask(
        grpc::ServerContext& context, const Proto::Task::StartTaskRequest& request)
    {
        Proto::Task::StartTaskReply reply;
        reply.set_started(true);

        console().writeLine("TaskRpcService::StartTask %s", request.codename().c_str());
        return reply;
    }

    Proto::Task::GetTargetReply TaskRpcService::GetTarget(
        grpc::ServerContext& context, const Proto::Task::GetTargetRequest& request)
    {
        Proto::Task::GetTargetReply reply;
        auto target = reply.mutable_target();
        target->set_id(request.id());

        auto pos = target->mutable_position();
        pos->set_longitude(100);
        pos->set_latitude(30);
        pos->set_height(5000);

        console().writeLine("TaskRpcService::GetTarget %d", request.id());
        return reply;
    }
}
