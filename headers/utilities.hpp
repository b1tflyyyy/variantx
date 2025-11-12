#pragma once

#include <cstddef>
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
        struct GetTypeIndexImpl;

        template <std::size_t Index, typename Target, typename Head, typename... Rest>
        struct GetTypeIndexImpl<SizeTWrapper<Index>, Target, Head, Rest...>
        {
            static constexpr std::size_t kValue =
                GetTypeIndexImpl<SizeTWrapper<(Index + 1)>, Target, Rest...>::kValue;
        };

        template <std::size_t Index, typename Target, typename... Rest>
        struct GetTypeIndexImpl<SizeTWrapper<Index>, Target, Target, Rest...>
        {
            static constexpr std::size_t kValue = Index;
        };
    }  // namespace detail

    template <typename Target, typename... Ts>
    // NOLINTNEXTLINE -> due to GetTypeIndex naming
    static constexpr std::size_t GetTypeIndex =
        detail::GetTypeIndexImpl<SizeTWrapper<0>, Target, Ts...>::kValue;

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

        template <typename From, typename... Ts>
        struct SelectConvertibleImpl
        {
            static void Select(...);
        };

        template <typename From, typename To>
            requires(NotNarrowingConversion<From, To>)
        struct SelectConvertibleImpl<From, To>
        {
            static To Select(To);
        };

        template <typename From, typename... Ts>
        struct SelectorImpl : SelectConvertibleImpl<From, Ts>...
        {
            using SelectConvertibleImpl<From, Ts>::Select...;
        };
    }  // namespace detail

    template <typename From, typename... Ts>
    using SelectorType = decltype(detail::SelectorImpl<From, Ts...>::Select(std::declval<From>()));

    template <typename From, typename... Ts>
    // NOLINTNEXTLINE
    static constexpr bool SelectorValue = !std::is_same_v<void, SelectorType<From, Ts...>>;
}  // namespace utilities
