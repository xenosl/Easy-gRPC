#pragma once

#include "ShuHai/gRPC/Client/Detail/AsyncUnaryCall.h"

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
            _stubs = std::make_tuple(std::make_unique<Stubs>(_channel)...);
            initCall();
        }

        ~AsyncClient() { deinitCall(); }

    private:
        std::shared_ptr<grpc::Channel> _channel;


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

        using StubTuple = std::tuple<std::unique_ptr<Stubs>...>;
        StubTuple _stubs;


        // Calls -------------------------------------------------------------------------------------------------------
    public:
        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Prepare<RpcName>.
         *  Get result by the returning std::future<ResponseType> object.
         * \param prepareFunc The function address of Stub::Prepare<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \return A std::future<ResponseType> object used to obtain the rpc result.
         */
        template<typename PrepareFunc>
        std::future<typename AsyncCallTraits<PrepareFunc>::ResponseType>
        call(PrepareFunc prepareFunc, const typename AsyncCallTraits<PrepareFunc>::RequestType& request,
            const std::function<void(grpc::ClientContext&)>& contextSetup = nullptr)
        {
            using Call = Detail::AsyncUnaryCall<PrepareFunc>;
            auto call = new Call();
            if (contextSetup)
                contextSetup(call->context());
            return call->start(stub<typename Call::Stub>(), prepareFunc, request, _callQueue.get());
        }

        /**
         * \brief Executes certain rpc via the specified generated function (which located in *.grpc.pb.h files) Stub::Prepare<RpcName>.
         *  Get notification and result by a callback function.
         * \param prepareFunc The function address of Stub::Prepare<RpcName> which need to be executed.
         * \param request The rpc parameter.
         * \param callback The callback function for rpc result notification.
         */
        template<typename PrepareFunc>
        void call(PrepareFunc prepareFunc, const typename AsyncCallTraits<PrepareFunc>::RequestType& request,
            typename Detail::AsyncUnaryCall<PrepareFunc>::ResultCallback callback,
            const std::function<void(grpc::ClientContext&)>& contextSetup = nullptr)
        {
            using Call = Detail::AsyncUnaryCall<PrepareFunc>;
            auto call = new Call();
            if (contextSetup)
                contextSetup(call->context());
            call->start(stub<typename Call::Stub>(), prepareFunc, request, _callQueue.get(), std::move(callback));
        }

    private:
        void initCall()
        {
            _callQueue = std::make_unique<grpc::CompletionQueue>();

            _callThread = std::make_unique<std::thread>(&AsyncClient::callQueueWorker, this);
        }

        void deinitCall()
        {
            _callQueue->Shutdown();

            _callThread->join();
            _callThread = nullptr;

            _callQueue = nullptr;
        }

        void callQueueWorker()
        {
            void* tag;
            bool ok;
            while (_callQueue->Next(&tag, &ok))
            {
                auto call = static_cast<Detail::AsyncCallBase*>(tag);
                if (ok)
                    call->finish();
                else
                    call->throws();
                delete call;
            }
        }

        std::unique_ptr<grpc::CompletionQueue> _callQueue;
        std::unique_ptr<std::thread> _callThread;
    };
}
