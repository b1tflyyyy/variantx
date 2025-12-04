#include <gtest/gtest.h>

#include <headers/variantx.hpp>

TEST(VariantxImpl, TraitsStaticAssertions) { namespace vi = variantx::impl; }

TEST(VariantxImpl, VariadicUnionStaticAssertions)
{
    namespace vi = variantx::impl;

    using V1 = vi::VariadicUnion<vi::Trait::Available, 0, int, float, double>;
    using V2 = vi::VariadicUnion<vi::Trait::TriviallyAvailable, 0, int, float, double>;
    using V3 = vi::VariadicUnion<vi::Trait::Unavailable, 0, int, float, double>;

    static_assert(std::is_destructible_v<V1> && !std::is_trivially_destructible_v<V1>);
    static_assert(std::is_destructible_v<V2> && std::is_trivially_destructible_v<V2>);
    static_assert(!std::is_destructible_v<V3> && !std::is_trivially_destructible_v<V3>);
}
