#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <utility>

namespace utilities
{
    template <std::size_t Value>
    struct SizeTWrapper
    {
    };

    namespace detail
    {
        template <typename... Ts>
        struct GetTypeByIndexImpl;

        template <std::size_t CurrentIndex, std::size_t TargetIndex, typename Head,
                  typename... Rest>
        struct GetTypeByIndexImpl<SizeTWrapper<CurrentIndex>, SizeTWrapper<TargetIndex>, Head,
                                  Rest...>
        {
            using Type = typename GetTypeByIndexImpl<SizeTWrapper<CurrentIndex + 1>,
                                                     SizeTWrapper<TargetIndex>, Rest...>::Type;
        };

        template <std::size_t CurrentIndex, typename Head, typename... Rest>
        struct GetTypeByIndexImpl<SizeTWrapper<CurrentIndex>, SizeTWrapper<CurrentIndex>, Head,
                                  Rest...>
        {
            using Type = Head;
        };
    }  // namespace detail

    template <std::size_t TargetIndex, typename... Ts>
    using GetTypeByIndex =
        typename detail::GetTypeByIndexImpl<SizeTWrapper<0>, SizeTWrapper<TargetIndex>,
                                            Ts...>::Type;

    namespace detail
    {
        /*
            https://en.cppreference.com/w/cpp/utility/variant/variant.html
            4) Converting constructor.
        */

        template <typename From, typename To>
        concept NotNarrowingConversion = requires(From&& from) {
            { std::type_identity_t<To[]>{std::forward<From>(from)} };  // NOLINT
        };

        template <typename To, std::size_t Index>
        struct Overload
        {
            template <typename From>
                requires NotNarrowingConversion<From, To>
            // type wrapper should be here
            std::type_identity<To> operator()(To, From&&) const;
        };

        template <typename... Overloads>
        struct AllOverloads : Overloads...
        {
            void operator()() const;
            using Overloads::operator()...;
        };

        template <typename Index>
        struct MakeAllOverloadsImpl;

        template <std::size_t... Indices>
        struct MakeAllOverloadsImpl<std::index_sequence<Indices...>>
        {
            template <typename... Ts>
            using Make = AllOverloads<Overload<Ts, Indices>...>;
        };

        template <typename... Ts>
        using MakeAllOverloads = typename MakeAllOverloadsImpl<
            std::make_index_sequence<sizeof...(Ts)>>::template Make<Ts...>;

        template <typename From, typename... Ts>
        using BestMatchType =
            typename std::invoke_result_t<MakeAllOverloads<Ts...>, From, From>::type;
    }  // namespace detail

    template <typename From, typename... Ts>
    using SelectorType = detail::BestMatchType<From, Ts...>;

    namespace detail
    {
        template <typename T>
        struct IsInplaceTypeImpl : std::false_type
        {
        };

        template <typename T>
        struct IsInplaceTypeImpl<std::in_place_type_t<T>> : std::true_type
        {
        };
    }  // namespace detail

    template <typename T>
    using IsInplaceType = detail::IsInplaceTypeImpl<std::remove_cvref_t<T>>;

    namespace detail
    {
        template <typename T>
        struct IsInplaceIndexImpl : std::false_type
        {
        };

        template <std::size_t Index>
        struct IsInplaceIndexImpl<std::in_place_index_t<Index>> : std::true_type
        {
        };
    }  // namespace detail

    template <typename T>
    using IsInplaceIndex = detail::IsInplaceIndexImpl<std::remove_cvref_t<T>>;

    namespace detail
    {
        static constexpr std::size_t kNotFound  = std::numeric_limits<std::size_t>::max();
        static constexpr std::size_t kAmbiguous = kNotFound - 1;

        template <std::size_t N>
        consteval std::size_t FindIndex(const std::array<bool, N>& array)
        {
            std::size_t index = kNotFound;
            for (std::size_t i = 0; i < std::size(array); ++i)
            {
                if (array[i])
                {
                    if (index != kNotFound)
                    {
                        return kAmbiguous;
                    }

                    index = i;
                }
            }

            return index;
        }

        template <typename T, typename... Ts>
        consteval std::size_t FindIndex()
        {
            constexpr std::array<bool, sizeof...(Ts)> kArray = {std::is_same_v<T, Ts>...};
            return FindIndex(kArray);
        }

        template <typename T, typename... Ts>
        struct FindExactlyOneImpl
        {
            static constexpr std::size_t kValue = FindIndex<T, Ts...>();

            static_assert(kValue != kNotFound, "type not found in type list");
            static_assert(kValue != kAmbiguous, "type occurs more than once in type list");
        };
    }  // namespace detail

    template <typename T, typename... Args>
    // NOLINTNEXTLINE
    static constexpr std::size_t FindExactlyOne = detail::FindExactlyOneImpl<T, Args...>::kValue;

    namespace detail
    {
        template <std::size_t Index>
        struct FindUnambiguousIndexImpl : std::integral_constant<std::size_t, Index>
        {
        };

        template <>
        struct FindUnambiguousIndexImpl<kNotFound>
        {
        };

        template <>
        struct FindUnambiguousIndexImpl<kAmbiguous>
        {
        };
    }  // namespace detail

    template <typename T, typename... Ts>
    struct FindUnambiguousIndex : detail::FindUnambiguousIndexImpl<detail::FindIndex<T, Ts...>()>
    {
    };
}  // namespace utilities
