#pragma once

#include <functional>
#include <type_traits>

namespace ShuHai::gRPC
{
    class IAsyncAction
    {
    public:
        virtual ~IAsyncAction() = default;

        virtual void perform() = 0;
        virtual void finalizeResult(bool ok) = 0;
    };

    class AsyncAction : public IAsyncAction
    {
    public:
        explicit AsyncAction(std::function<void(bool)> finalizer = nullptr)
            : _finalizer(std::move(finalizer))
        { }

        void perform() override { }

        ~AsyncAction() override = default;

        void finalizeResult(bool ok) override
        {
            if (_finalizer)
                _finalizer(ok);
        }

    private:
        std::function<void(bool ok)> _finalizer;
    };

    template<typename T, typename... Args>
    std::enable_if_t<std::is_base_of_v<IAsyncAction, T>, void> performNewAction(Args&&... args)
    {
        auto action = new T(std::forward<Args>(args)...);
        action->perform();
    }
}
