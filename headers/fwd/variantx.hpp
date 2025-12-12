#pragma once

#include <cstddef>
#include <numeric>

namespace variantx
{
    template <typename... Ts>
    class Variant;

    template <typename T>
    struct VariantSize;

    template <typename T>
    inline constexpr std::size_t kVariantSizeV = VariantSize<T>::kValue;

    template <std::size_t Index, typename T>
    struct VariantAlternative;

    template <std::size_t Index, typename T>
    using VariantAlternativeType = typename VariantAlternative<Index, T>::Type;

    inline constexpr std::size_t kVariantNpos = std::numeric_limits<std::size_t>::max();

    // TODO:
    // add Get<...>(...)
}  // namespace variantx
