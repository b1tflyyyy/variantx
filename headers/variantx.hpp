#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>
#include <utilities.hpp>
#include <variantx-exceptions.hpp>

namespace variantx
{
    // NOLINTNEXTLINE
    inline constexpr std::size_t variant_npos = std::numeric_limits<std::size_t>::max();

    template <std::size_t Index, typename T>
    struct VariantAlternative;

    template <std::size_t Index, typename T>
    using VariantAlternativeType = typename VariantAlternative<Index, T>::Type;

    template <typename... Ts>
    class Variant
    {
        template <std::size_t Index, typename... Args>
        friend constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Args...>>> GetIf(
            Variant<Args...>* variant) noexcept;

        template <std::size_t Index, typename... Args>
        friend constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Args...>>>
        GetIf(const Variant<Args...>* variant) noexcept;

        template <typename Target, typename... Args>
        friend constexpr std::add_pointer_t<Target> GetIf(Variant<Args...>* variant) noexcept;

        template <typename Target, typename... Args>
        friend constexpr std::add_pointer_t<const Target> GetIf(
            const Variant<Args...>* variant) noexcept;

    public:
        constexpr Variant() noexcept : index_(0)
        {
            using Type = utilities::GetTypeByIndex<0, Ts...>;
            static_assert(std::is_nothrow_constructible_v<Type>,
                          "Type at index 0 is not nothrow default constructible!");

            new (std::data(storage_)) Type();
        }

        template <typename T>
        constexpr explicit Variant(T&& object)
            : index_(utilities::GetTypeIndex<std::decay_t<T>, Ts...>)
        {
            using Decayed = std::decay_t<T>;
            static_assert((std::is_same_v<Decayed, Ts> || ...), "Variant does not contain type T");

            new (std::data(storage_)) Decayed(std::forward<T>(object));
        }

        // TODO:
        constexpr explicit Variant(const Variant<Ts...>&)     = delete;
        constexpr explicit Variant(Variant<Ts...>&&) noexcept = delete;

        // TODO:
        Variant& operator=(const Variant<Ts...>&)     = delete;
        Variant& operator=(Variant<Ts...>&&) noexcept = delete;

        constexpr ~Variant() noexcept { Destroy<0>(); }

        constexpr bool ValuelessByException() const noexcept { return index_ == variant_npos; }

        template <typename Self>
        std::size_t Index(this Self&& self)
        {
            return std::forward<Self>(self).index_;
        }

        template <std::size_t Index, typename... Args>
        VariantAlternativeType<Index, Variant<Ts...>>& Emplace(Args&&... args)
        {
            using Type = utilities::GetTypeByIndex<Index, Ts...>;

            Destroy<0>();
            Construct<Type>(std::forward<Args>(args)...);

            index_ = Index;
            return *Cast<Type>();
        }

        template <typename T, typename... Args>
        T& Emplace(Args&&... args)
        {
            constexpr auto index = utilities::GetTypeIndex<T, Ts...>;  // NOLINT
            return Emplace<index>(std::forward<Args>(args)...);
        }

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

        template <std::size_t Index>
        constexpr void Destroy() noexcept
        {
            if (Index == index_)
            {
                using Type = utilities::GetTypeByIndex<Index, Ts...>;
                auto* ptr  = Cast<Type>();

                ptr->~Type();
            }
            else
            {
                Destroy<Index + 1>();
            }
        }

        template <>
        constexpr void Destroy<sizeof...(Ts)>() noexcept
        {
        }

        template <typename T, typename... Args>
        void Construct(Args&&... args)
        {
            try
            {
                new (std::data(storage_)) T(std::forward<Args>(args)...);
            }
            catch (...)
            {
                index_ = variant_npos;
                throw;
            }
        }

        /*
        template <std::size_t Index>
        constexpr void CopyConstruct(const Variant& rhs)
        {
        }
        */

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
        static_assert(false,
                      "Deprecated since C++20 "
                      "https://en.cppreference.com/w/cpp/utility/variant/variant_alternative.html");
    };

    template <std::size_t Index, typename... Ts>
    struct VariantAlternative<Index, const volatile Variant<Ts...>>
    {
        static_assert(false,
                      "Deprecated since C++20 "
                      "https://en.cppreference.com/w/cpp/utility/variant/variant_alternative.html");
    };

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        Variant<Ts...>* variant) noexcept
    {
        if (variant == nullptr || variant->index_ != Index)
        {
            return nullptr;
        }

        using Type = VariantAlternativeType<Index, Variant<Ts...>>;
        return variant->template Cast<Type>();
    }

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        const Variant<Ts...>* variant) noexcept
    {
        if (variant == nullptr || variant->index_ != Index)
        {
            return nullptr;
        }

        using Type = VariantAlternativeType<Index, Variant<Ts...>>;
        return variant->template ConstCast<Type>();
    }

    template <typename Target, typename... Ts>
    constexpr std::add_pointer_t<Target> GetIf(Variant<Ts...>* variant) noexcept
    {
        return GetIf<utilities::GetTypeIndex<Target, Ts...>>(variant);
    }

    template <typename Target, typename... Ts>
    constexpr std::add_pointer_t<const Target> GetIf(const Variant<Ts...>* variant) noexcept
    {
        return GetIf<utilities::GetTypeIndex<Target, Ts...>>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>& Get(Variant<Ts...>& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return *ptr;
    }

    template <std::size_t Index, typename... Ts>
    // NOLINTNEXTLINE -> variant is never moved inside
    constexpr VariantAlternativeType<Index, Variant<Ts...>>&& Get(Variant<Ts...>&& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return std::move(*ptr);
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>& Get(
        const Variant<Ts...>& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return *ptr;
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>&& Get(
        const Variant<Ts...>&& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return std::move(*ptr);
    }

    template <typename T, typename... Ts>
    constexpr T& Get(Variant<Ts...>& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr T&& Get(Variant<Ts...>&& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(std::move(variant));
    }

    template <typename T, typename... Ts>
    constexpr const T& Get(const Variant<Ts...>& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr const T&& Get(const Variant<Ts...>&& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(std::move(variant));
    }
}  // namespace variantx
