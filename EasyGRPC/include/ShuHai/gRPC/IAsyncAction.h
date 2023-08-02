#pragma once

#include <future>
#include <functional>
#include <type_traits>

namespace ShuHai::gRPC
{
    class IAsyncAction
    {
    public:
        virtual ~IAsyncAction() = default;

        virtual void finalizeResult(bool ok) = 0;
    };

    template<typename T>
    class AsyncAction : public IAsyncAction
    {
    public:
        std::future<T> result() { return _resultPromise.get_future(); }

        void setResult(T result) { _resultPromise.set_value(result); }

        template<typename E, typename... Args>
        void setException(Args... args)
        {
            try
            {
                throw E(std::forward<Args>(args)...);
            }
            catch (...)
            {
                _resultPromise.set_exception(std::current_exception());
            }
        }

    private:
        std::promise<T> _resultPromise;
    };

    template class AsyncAction<bool>;
}
