#pragma once

#include "ShuHai/gRPC/Client/AsyncUnaryCall.h"
#include "ShuHai/gRPC/Client/AsyncClientStreamCall.h"
#include "ShuHai/gRPC/AsyncActionQueue.h"

#include <grpcpp/grpcpp.h>

#include <thread>
#include <future>
#include <unordered_set>
#include <tuple>

namespace ShuHai::gRPC::Client
{
    template<typename... Stubs>
    class AsyncClient
    {
    public:
        explicit AsyncClient(const std::string& targetEndpoint,
            std::shared_ptr<grpc::ChannelCredentials> credentials = grpc::InsecureChannelCredentials(),
            const grpc::ChannelArguments& channelArguments = {})
        {
            _channel = grpc::CreateCustomChannel(targetEndpoint, credentials, channelArguments);
            initStubs();
            initAsyncActionQueue();
        }

        ~AsyncClient() { deinitAsyncActionQueue(); }

    private:
        std::shared_ptr<grpc::Channel> _channel;


        // Calls -------------------------------------------------------------------------------------------------------
    public:
        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Async<RpcName>.
         *  Get result by function AsyncUnaryCall<CallFunc>::response() of the returned call instance.
         * \param asyncCall The function address of Stub::Async<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \return The call instance.
         */
        template<typename CallFunc>
        EnableIfRpcTypeMatch<CallFunc, RpcType::UnaryCall, std::shared_ptr<AsyncUnaryCall<CallFunc>>> call(
            CallFunc asyncCall, const RequestTypeOf<CallFunc>& request,
            std::unique_ptr<grpc::ClientContext> context = nullptr)
        {
            return call(asyncCall, request, nullptr, nullptr, std::move(context));
        }

        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Async<RpcName>.
         *  Get notification and result by a callback function.
         * \param asyncCall The function address of Stub::Async<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \param callback The callback function for rpc result notification.
         * \param callbackExecutionContext The asio execution context that execute the specified callback function. System
         *  context is used if the value is null.
         * \param context The gRPC context for the call.
         * \return The call instance.
         */
        template<typename CallFunc>
        EnableIfRpcTypeMatch<CallFunc, RpcType::UnaryCall, std::shared_ptr<AsyncUnaryCall<CallFunc>>> call(
            CallFunc asyncCall, const RequestTypeOf<CallFunc>& request,
            typename AsyncUnaryCall<CallFunc>::ResponseCallback callback,
            asio::execution_context* callbackExecutionContext = nullptr,
            std::unique_ptr<grpc::ClientContext> context = nullptr)
        {
            using Call = AsyncUnaryCall<CallFunc>;
            auto call = std::make_shared<Call>(stub<typename Call::Stub>(), asyncCall, request,
                _asyncActionQueue->completionQueue(), std::move(context), std::move(callback), callbackExecutionContext,
                [this](std::shared_ptr<Call> c) { onCallDead(c); });
            addStreamingCall(call);
            return call;
        }

        template<typename CallFunc>
        EnableIfRpcTypeMatch<CallFunc, RpcType::ClientStream, std::shared_ptr<AsyncClientStreamCall<CallFunc>>> call(
            CallFunc asyncCall, std::unique_ptr<grpc::ClientContext> context = nullptr)
        {
            using Call = AsyncClientStreamCall<CallFunc>;
            auto call = std::make_shared<Call>(
                stub<typename Call::Stub>(), asyncCall, std::move(context), _asyncActionQueue->completionQueue());
            addStreamingCall(call);
            return call;
        }

    private:
        using CallPtr = std::shared_ptr<AsyncCall>;

        void onCallDead(CallPtr call) { removeStreamingCall(call); }

        void addStreamingCall(CallPtr call)
        {
            std::lock_guard l(_streamingCallsMutex);
            _streamingCalls.emplace(call);
        }

        void removeStreamingCall(CallPtr call)
        {
            std::lock_guard l(_streamingCallsMutex);
            _streamingCalls.erase(call);
        }

        std::mutex _streamingCallsMutex;
        std::unordered_set<CallPtr> _streamingCalls;

        // Action Queue ------------------------------------------------------------------------------------------------
    private:
        void initAsyncActionQueue()
        {
            _asyncActionQueue = std::make_unique<AsyncActionQueue>(std::make_unique<grpc::CompletionQueue>());

            _asyncActionThread = std::thread(
                [this]()
                {
                    while (_asyncActionQueue->asyncNext() != grpc::CompletionQueue::SHUTDOWN)
                        continue;
                });
        }

        void deinitAsyncActionQueue()
        {
            _asyncActionQueue->shutdown();

            _asyncActionThread.join();

            _asyncActionQueue = nullptr;
        }

        std::unique_ptr<AsyncActionQueue> _asyncActionQueue;
        std::thread _asyncActionThread;


        // Stubs -------------------------------------------------------------------------------------------------------
    public:
        using StubIndices = std::index_sequence_for<Stubs...>;
        static constexpr size_t StubCount = StubIndices::size();
        static_assert(StubCount > 0);

        template<typename Stub>
        static constexpr bool isSupportedStubType()
        {
            return ((std::is_same_v<Stub, Stubs>) || ...);
        }

        template<typename Stub>
        [[nodiscard]] Stub* stub() const
        {
            return std::get<indexOfStub<Stub>()>(_stubs).get();
        }

    private:
        template<typename Stub>
        static constexpr size_t indexOfStub()
        {
            static_assert(isSupportedStubType<Stub>());
            return indexOfStubImpl<0, Stub>();
        }

        template<size_t I, typename Stub>
        static constexpr size_t indexOfStubImpl()
        {
            static_assert(I < StubCount);
            if constexpr (std::is_same_v<Stub, typename std::tuple_element_t<I, StubTuple>::element_type>)
                return I;
            else
                return indexOfStubImpl<I + 1, Stub>();
        }

        void initStubs() { _stubs = std::make_tuple(std::make_unique<Stubs>(_channel)...); }

        using StubTuple = std::tuple<std::unique_ptr<Stubs>...>;
        StubTuple _stubs;
    };
}
