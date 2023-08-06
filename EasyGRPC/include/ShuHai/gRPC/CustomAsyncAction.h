#pragma once

#include "ShuHai/gRPC/IAsyncAction.h"

#include <grpcpp/alarm.h>

namespace ShuHai::gRPC
{
    /**
     * \brief Base class for non-gRPC actions.
     */
    class CustomAsyncAction : public IAsyncAction
    {
    public:
        void trigger(grpc::CompletionQueue* cq) { trigger(cq, gpr_now(GPR_CLOCK_REALTIME)); }

        template<typename Time>
        void trigger(grpc::CompletionQueue* cq, const Time& deadline)
        {
            _alarm.Set(cq, deadline, this);
        }

    private:
        grpc::Alarm _alarm;
    };
}
