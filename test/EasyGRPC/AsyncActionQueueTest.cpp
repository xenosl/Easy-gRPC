#include "ShuHai/gRPC/AsyncActionQueue.h"

#include <grpcpp/alarm.h>

#include <gtest/gtest.h>

namespace ShuHai::gRPC::Test
{
    class AsyncActionQueueTest : public testing::Test
    {
    public:
        class CustomAction : public IAsyncAction
        {
        public:
            enum class State
            {
                Ready,
                Performing,
                Finished
            };

            void perform(grpc::CompletionQueue* cq)
            {
                assert(_state == State::Ready);
                _alarm.Set(cq, gpr_now(GPR_CLOCK_REALTIME), this);
                _state = State::Performing;
            }

            void finalizeResult(bool ok) override
            {
                _state = State::Finished;
                if (onFinished)
                    onFinished(this);
            }

            [[nodiscard]] State state() const { return _state; }

            std::function<void(CustomAction*)> onFinished;

        private:
            grpc::Alarm _alarm;
            State _state = State::Ready;
        };
    };

    TEST_F(AsyncActionQueueTest, AsyncActionShouldMatch)
    {
        AsyncActionQueue w(std::make_unique<grpc::CompletionQueue>());
        auto cq = w.completionQueue();

        auto action = new CustomAction();
        action->onFinished = [](auto a) { EXPECT_EQ(a->state(), CustomAction::State::Finished); };
        EXPECT_EQ(action->state(), CustomAction::State::Ready);
        action->perform(cq);
        EXPECT_EQ(action->state(), CustomAction::State::Performing);
        w.asyncNext(gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_millis(100, GPR_TIMESPAN)));
        w.shutdown();
    }
}
