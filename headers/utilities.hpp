#pragma once

#include <cstddef>

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
        struct GetTypeByIndexImpl<SizeTWrapper<CurrentIndex>, SizeTWrapper<TargetIndex>,
                                  Head, Rest...>
        {
            using Type =
                typename GetTypeByIndexImpl<SizeTWrapper<CurrentIndex + 1>,
                                            SizeTWrapper<TargetIndex>, Rest...>::Type;
        };

        template <std::size_t CurrentIndex, typename Head, typename... Rest>
        struct GetTypeByIndexImpl<SizeTWrapper<CurrentIndex>, SizeTWrapper<CurrentIndex>,
                                  Head, Rest...>
        {
            using Type = Head;
        };
    }  // namespace detail

    template <std::size_t TargetIndex, typename... Ts>
    using GetTypeByIndex =
        typename detail::GetTypeByIndexImpl<SizeTWrapper<0>, SizeTWrapper<TargetIndex>,
                                            Ts...>::Type;
}  // namespace utilities
