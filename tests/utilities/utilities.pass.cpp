#include <gtest/gtest.h>

#include <headers/utilities.hpp>
#include <type_traits>
#include <utility>

template <typename...>
struct List
{
};

TEST(Utilities, StaticAsserts)
{
    namespace ut = utilities;

    using L1            = List<int, float, double, char>;
    using IndexSequence = std::make_index_sequence<4>;

    std::invoke(
        []<std::size_t... Indices, typename... Args>(
            [[maybe_unused]] std::index_sequence<Indices...>&&, [[maybe_unused]] List<Args...>&&)
        {
            static_assert((std::is_same_v<Args, ut::GetTypeByIndex<Indices, Args...>> && ...));
            static_assert(((Indices == ut::GetTypeIndex<Args, Args...>) && ...));
        },
        IndexSequence{}, L1{});

    // TODO: add asserts for Selector
}
