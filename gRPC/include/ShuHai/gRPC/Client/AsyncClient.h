#pragma once

#include "ShuHai/gRPC/Client/AsyncUnaryCall.h"
#include "ShuHai/gRPC/Client/AsyncServerStream.h"
#include "ShuHai/gRPC/CompletionQueueWorker.h"

#include <grpcpp/grpcpp.h>

#include <thread>
#include <future>
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
            initCompletionQueue();
        }

        ~AsyncClient() { deinitCompletionQueue(); }

    private:
        std::shared_ptr<grpc::Channel> _channel;


        // Calls -------------------------------------------------------------------------------------------------------
    public:
        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Async<RpcName>.
         *  Get result by the returning std::future<ResponseType> object.
         * \param asyncCall The function address of Stub::Async<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \return A std::future<ResponseType> object used to obtain the rpc result.
         */
        template<typename AsyncCall>
        EnableIfRpcTypeMatch<AsyncCall, RpcType::NORMAL_RPC,
            std::future<typename AsyncCallTraits<AsyncCall>::ResponseType>>
        call(AsyncCall asyncCall, const typename AsyncCallTraits<AsyncCall>::RequestType& request,
            const std::function<void(grpc::ClientContext&)>& contextSetup = nullptr)
        {
            using Call = AsyncUnaryCall<AsyncCall>;
            auto call = new Call();
            if (contextSetup)
                contextSetup(call->context);
            return call->invoke(stub<typename Call::Stub>(), asyncCall, request, _cqWorker->queue());
        }

        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Async<RpcName>.
         *  Get notification and result by a callback function.
         * \param asyncCall The function address of Stub::Async<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \param callback The callback function for rpc result notification.
         */
        template<typename AsyncCall>
        EnableIfRpcTypeMatch<AsyncCall, RpcType::NORMAL_RPC, void> call(AsyncCall asyncCall,
            const typename AsyncCallTraits<AsyncCall>::RequestType& request,
            typename AsyncUnaryCall<AsyncCall>::ResultCallback callback,
            const std::function<void(grpc::ClientContext&)>& contextSetup = nullptr)
        {
            using Call = AsyncUnaryCall<AsyncCall>;
            auto call = new Call();
            if (contextSetup)
                contextSetup(call->context);
            call->invoke(stub<typename Call::Stub>(), asyncCall, request, _cqWorker->queue(), std::move(callback));
        }

        template<typename AsyncCall>
        EnableIfRpcTypeMatch<AsyncCall, RpcType::SERVER_STREAMING, std::shared_ptr<AsyncServerStream<AsyncCall>>> call(
            AsyncCall asyncCall, const typename AsyncCallTraits<AsyncCall>::RequestType& request,
            typename AsyncServerStream<AsyncCall>::ResponseCallback callback)
        {
            using Call = AsyncServerStream<AsyncCall>;
            auto call = std::make_shared<Call>(stub<typename Call::Stub>(), asyncCall, request, _cqWorker->queue(),
                std::move(callback), [this](auto c) { removeStreamingCall(c); });
            addStreamingCall(call);
            return call;
        }

    private:
        using CallPtr = std::shared_ptr<AsyncCallBase>;

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


        // Queue Worker ------------------------------------------------------------------------------------------------
    private:
        using CqWorker = CompletionQueueWorker<grpc::CompletionQueue>;

        void initCompletionQueue()
        {
            _cqWorker = std::make_unique<CqWorker>(std::make_unique<grpc::CompletionQueue>());

            _cqThread = std::thread(
                [this]()
                {
                    while (true)
                    {
                        if (!_cqWorker->poll())
                            break;
                    }
                });
        }

        void deinitCompletionQueue()
        {
            _cqWorker->shutdown();

            _cqThread.join();

            _cqWorker = nullptr;
        }

        std::unique_ptr<CqWorker> _cqWorker;
        std::thread _cqThread;


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
