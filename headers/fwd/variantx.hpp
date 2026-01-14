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

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>& Get(Variant<Ts...>& variant);

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>&& Get(Variant<Ts...>&& variant);

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>& Get(
        const Variant<Ts...>& variant);

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>&& Get(
        const Variant<Ts...>&& variant);

    template <typename T, typename... Ts>
    constexpr T& Get(Variant<Ts...>& variant);

    template <typename T, typename... Ts>
    constexpr T&& Get(Variant<Ts...>&& variant);

    template <typename T, typename... Ts>
    constexpr const T& Get(const Variant<Ts...>& variant);

    template <typename T, typename... Ts>
    constexpr const T&& Get(const Variant<Ts...>&& variant);

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        Variant<Ts...>* variant) noexcept;

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        const Variant<Ts...>* variant) noexcept;

    template <typename T, typename... Ts>
    constexpr std::add_pointer_t<T> GetIf(Variant<Ts...>* variant) noexcept;

    template <typename T, typename... Ts>
    constexpr std::add_pointer_t<const T> GetIf(const Variant<Ts...>* variant) noexcept;
}  // namespace variantx
