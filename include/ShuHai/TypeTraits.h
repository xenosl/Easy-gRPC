#pragma once

#include <tuple>
#include <type_traits>

namespace ShuHai
{
    template<typename T>
    using RemoveConstReferenceT = std::remove_const_t<std::remove_reference_t<T>>;
}
