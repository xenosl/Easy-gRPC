#pragma once

#include "ShuHai/gRPC/IAsyncAction.h"

namespace ShuHai::gRPC
{
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

    template<>
    class AsyncAction<void> : public IAsyncAction
    {
    public:
        std::future<void> result() { return _resultPromise.get_future(); }

        void setResult() { _resultPromise.set_value(); }

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
        std::promise<void> _resultPromise;
    };

    template class AsyncAction<bool>;
}
