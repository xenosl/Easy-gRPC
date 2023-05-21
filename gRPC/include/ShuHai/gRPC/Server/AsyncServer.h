#pragma once

#include "ShuHai/gRPC/Server/CompletionQueueWorker.h"

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
        explicit AsyncServer(const std::vector<std::string>& listeningUris, size_t numCompletionQueues = 1)
        {
            if (listeningUris.empty())
                throw std::invalid_argument("At least one listening uri is required.");

            if (numCompletionQueues == 0)
                numCompletionQueues = std::thread::hardware_concurrency();

            grpc::ServerBuilder builder;
            for (const auto& uri : listeningUris)
                builder.AddListeningPort(uri, grpc::InsecureServerCredentials());
            foreachService([&](auto s) { builder.RegisterService(s); });

            for (size_t i = 0; i < numCompletionQueues; ++i)
                _queueWorkers.emplace_back(std::make_unique<CompletionQueueWorker>(builder.AddCompletionQueue()));

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

            for (auto& queue : _queueWorkers)
            {
                auto t = std::make_unique<std::thread>(
                    [&]()
                    {
                        while (true)
                        {
                            if (!queue->poll())
                                break;
                        }
                    });
                _queueThreads.emplace_back(std::move(t));
            }

            _started = true;
        }

        void stop()
        {
            if (!started())
                return;

            _server->Shutdown();
            for (auto& w : _queueWorkers)
                w->shutdown();

            for (auto& t : _queueThreads)
                t->join();
            _queueThreads.clear();

            _server = nullptr;
            _queueWorkers.clear();
        }

        [[nodiscard]] bool started() const { return _started; }

    private:
        std::atomic_bool _started { false };

        std::unique_ptr<grpc::Server> _server;

        std::vector<std::unique_ptr<CompletionQueueWorker>> _queueWorkers;
        std::vector<std::unique_ptr<std::thread>> _queueThreads;


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
            constexpr bool match = CheckDerived ? std::is_base_of_v<Service, std::tuple_element_t<I, ServiceTuple>>
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
         * \param processFunc The function actually take care of the rpc call.
         */
        template<typename RequestFunc>
        EnableIfRpcTypeMatch<RequestFunc, RpcType::NORMAL_RPC, void> registerCallHandler(RequestFunc requestFunc,
            typename AsyncUnaryCallHandler<RequestFunc>::ProcessFunc processFunc, size_t queueIndex = 0)
        {
            using Service = typename AsyncRequestTraits<RequestFunc>::ServiceType;
            auto& w = _queueWorkers.at(queueIndex);
            w->registerCallHandler(requestFunc, this->service<Service>(), std::move(processFunc));
        }

        template<typename RequestFunc>
        EnableIfRpcTypeMatch<RequestFunc, RpcType::SERVER_STREAMING, void> registerCallHandler(RequestFunc requestFunc,
            typename AsyncServerStreamHandler<RequestFunc>::ProcessFunc processFunc, size_t queueIndex = 0)
        {
            using Service = typename AsyncRequestTraits<RequestFunc>::ServiceType;
            auto& w = _queueWorkers.at(queueIndex);
            w->registerCallHandler(requestFunc, this->service<Service>(), std::move(processFunc));
        }

        template<typename RequestFunc>
        EnableIfRpcTypeMatch<RequestFunc, RpcType::CLIENT_STREAMING, void> registerCallHandler(RequestFunc requestFunc,
            typename AsyncClientStreamCallHandler<RequestFunc>::ProcessFunc processFunc, size_t queueIndex = 0)
        {
            using Service = typename AsyncRequestTraits<RequestFunc>::ServiceType;
            auto& w = _queueWorkers.at(queueIndex);
            w->registerCallHandler(requestFunc, this->service<Service>(), std::move(processFunc));
        }
    };
}
