#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utilities.hpp>

namespace variantx
{
    template <std::size_t Index, typename T>
    struct VariantAlternative;

    template <std::size_t Index, typename T>
    using VariantAlternativeType = typename VariantAlternative<Index, T>::Type;

    template <typename... Ts>
    class Variant
    {
        template <std::size_t Index, typename... Args>
        friend constexpr std::add_pointer_t<
            VariantAlternativeType<Index, Variant<Args...>>>
        GetIf(Variant<Args...>* variant) noexcept;

        template <std::size_t Index, typename... Args>
        friend constexpr std::add_pointer_t<
            const VariantAlternativeType<Index, Variant<Args...>>>
        GetIf(const Variant<Args...>* variant) noexcept;

        template <typename Target, typename... Args>
        friend constexpr std::add_pointer_t<Target> GetIf(
            Variant<Args...>* variant) noexcept;

        template <typename Target, typename... Args>
        friend constexpr std::add_pointer_t<const Target> GetIf(
            const Variant<Args...>* variant) noexcept;

    public:
        template <typename T>
        constexpr explicit Variant(T&& object)
            : index_(utilities::GetTypeIndex<std::decay_t<T>, Ts...>)
        {
            static_assert((std::is_same_v<std::decay_t<T>, Ts> || ...),
                          "Variant does not contain type T");

            new (std::data(storage_)) std::decay_t<T>(std::forward<T>(object));
        }

        // TODO:
        constexpr explicit Variant(const Variant<Ts...>&) = delete;
        constexpr explicit Variant(Variant<Ts...>&&) noexcept = delete;

        // TODO:
        Variant& operator=(const Variant<Ts...>&) = delete;
        Variant& operator=(Variant<Ts...>&&) noexcept = delete;

        // TODO:
        ~Variant() noexcept = default;

    private:  // NOLINT -> for readability
        // TODO:
        template <typename T>
        constexpr T* Cast()
        {
            // NOLINTNEXTLINE -> reinterpret_cast using
            return std::launder(reinterpret_cast<T*>(std::data(storage_)));
        }

        // TODO:
        template <typename T>
        constexpr const T* ConstCast() const
        {
            // NOLINTNEXTLINE -> reinterpret_cast using
            return std::launder(reinterpret_cast<const T*>(std::data(storage_)));
        }

    private:  // NOLINT -> for readability
        std::size_t index_;
        alignas(Ts...) std::array<std::byte, std::max({sizeof(Ts)...})> storage_;
    };

    template <std::size_t Index, typename... Ts>
    struct VariantAlternative<Index, Variant<Ts...>>
    {
        static_assert(Index < sizeof...(Ts), "Index out of variant range!");
        using Type = utilities::GetTypeByIndex<Index, Ts...>;
    };

    template <std::size_t Index, typename... Ts>
    struct VariantAlternative<Index, const Variant<Ts...>>
    {
        static_assert(Index < sizeof...(Ts), "Index out of variant range!");
        using Type = std::add_const_t<utilities::GetTypeByIndex<Index, Ts...>>;
    };

    template <std::size_t Index, typename... Ts>
    struct VariantAlternative<Index, volatile Variant<Ts...>>
    {
        static_assert(
            false,
            "Deprecated since C++20 "
            "https://en.cppreference.com/w/cpp/utility/variant/variant_alternative.html");
    };

    template <std::size_t Index, typename... Ts>
    struct VariantAlternative<Index, const volatile Variant<Ts...>>
    {
        static_assert(
            false,
            "Deprecated since C++20 "
            "https://en.cppreference.com/w/cpp/utility/variant/variant_alternative.html");
    };

    template <std::size_t Index, typename... Args>
    constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Args...>>> GetIf(
        Variant<Args...>* variant) noexcept
    {
        if (variant == nullptr || variant->index_ != Index)
        {
            return nullptr;
        }

        using Type = VariantAlternativeType<Index, Variant<Args...>>;
        return variant->template Cast<Type>();
    }

    template <std::size_t Index, typename... Args>
    constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Args...>>>
    GetIf(const Variant<Args...>* variant) noexcept
    {
        if (variant == nullptr || variant->index_ != Index)
        {
            return nullptr;
        }

        using Type = VariantAlternativeType<Index, Variant<Args...>>;
        return variant->template ConstCast<Type>();
    }

    template <typename Target, typename... Args>
    constexpr std::add_pointer_t<Target> GetIf(Variant<Args...>* variant) noexcept
    {
        return GetIf<utilities::GetTypeIndex<Target, Args...>>(variant);
    }

    template <typename Target, typename... Args>
    constexpr std::add_pointer_t<const Target> GetIf(
        const Variant<Args...>* variant) noexcept
    {
        return GetIf<utilities::GetTypeIndex<Target, Args...>>(variant);
    }
}  // namespace variantx
