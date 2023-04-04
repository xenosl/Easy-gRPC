#pragma once

#include "ShuHai/gRPC/Server/CompletionQueue.h"
#include "ShuHai/gRPC/Server/Detail/AsyncUnaryCallHandler.h"

#include <grpcpp/grpcpp.h>

#include <thread>
#include <unordered_set>
#include <vector>
#include <utility>
#include <type_traits>

namespace ShuHai::gRPC::Server
{
    template<typename... Services>
    class AsyncServer
    {
    public:
        explicit AsyncServer(const std::vector<std::string>& listeningUris)
        {
            if (listeningUris.empty())
                throw std::invalid_argument("At least one listening uri is required.");

            grpc::ServerBuilder builder;
            for (const auto& uri : listeningUris)
                builder.AddListeningPort(uri, grpc::InsecureServerCredentials());
            foreachService([&](auto s) { builder.RegisterService(s); });

            _queue = std::make_unique<CompletionQueue>(builder.AddCompletionQueue());

            _server = builder.BuildAndStart();
            if (!_server)
                throw std::logic_error("Build rpc server failed.");
        }

        explicit AsyncServer(uint16_t port)
            : AsyncServer(std::vector<std::string> { "0.0.0.0:" + std::to_string(port) })
        { }

        ~AsyncServer() { stop(); }

        void start()
        {
            if (started())
                throw std::logic_error("The server already started.");

            _queueThread = std::make_unique<std::thread>(&AsyncServer::queueWorker, this);
        }

        void stop()
        {
            if (!_queueThread)
                return;

            _server->Shutdown();
            _queue->shutdown();

            _queueThread->join();
            _queueThread = nullptr;

            _server = nullptr;
            _queue = nullptr;
        }

        [[nodiscard]] bool started() const { return (bool)_queueThread; }

    private:
        std::unique_ptr<grpc::Server> _server;
        std::unique_ptr<CompletionQueue> _queue;

        void queueWorker()
        {
            while (true)
            {
                if (!_queue->poll())
                    break;
            }
        }

        std::unique_ptr<std::thread> _queueThread;


        // Services ----------------------------------------------------------------------------------------------------
    public:
        using ServiceIndices = std::index_sequence_for<Services...>;
        static constexpr size_t ServiceCount = ServiceIndices::size();
        static_assert(ServiceCount > 0);

        template<typename Service, bool CheckDerived = true>
        static constexpr bool isSupportedServiceType()
        {
            return ((CheckDerived ? std::is_base_of_v<Service, Services> : std::is_same_v<Service, Services>) || ...);
        }

        template<typename Service>
        Service* service()
        {
            return &std::get<indexOfService<Service>()>(_services);
        }

    private:
        template<typename Service, bool CheckDerived = true>
        static constexpr size_t indexOfService()
        {
            static_assert(isSupportedServiceType<Service, CheckDerived>());
            return indexOfServiceImpl<0, Service, CheckDerived>();
        }

        template<size_t I, typename Service, bool CheckDerived>
        static constexpr size_t indexOfServiceImpl()
        {
            static_assert(I < ServiceCount);
            constexpr bool match =
                CheckDerived
                    ? std::is_base_of_v<Service, std::tuple_element_t<I, ServiceTuple>>
                    : std::is_same_v<Service, std::tuple_element_t<I, ServiceTuple>>;
            if constexpr (match)
                return I;
            else
                return indexOfServiceImpl<I + 1, Service, CheckDerived>();
        }

        void foreachService(const std::function<void(grpc::Service*)>& func)
        {
            foreachServiceImpl(func, ServiceIndices {});
        }

        template<size_t... Indices>
        void foreachServiceImpl(const std::function<void(grpc::Service*)>& func, std::index_sequence<Indices...>)
        {
            (func(&std::get<Indices>(_services)), ...);
        }

        using ServiceTuple = std::tuple<Services...>;
        ServiceTuple _services;


        // Call Handlers -----------------------------------------------------------------------------------------------
    public:
        /**
         * \brief Register certain rpc call handler corresponding to the specified function AsyncService::Request<RpcName>
         *  located in the generated code.
         * \param requestFunc Function address of AsyncService::Request<RpcName> located in generated code.
         * \param processFunc The function actually take care of the rpc request.
         */
        template<typename RequestFunc>
        void registerCallHandler(RequestFunc requestFunc,
            std::function<void(const typename AsyncRequestFuncTraits<RequestFunc>::RequestType&,
                typename AsyncRequestFuncTraits<RequestFunc>::ResponseType&)> processFunc)
        {
            using Service = typename AsyncRequestFuncTraits<RequestFunc>::ServiceType;
            _queue->registerCallHandler(this->service<Service>(), requestFunc, std::move(processFunc));
        }
    };
}
