#include <gtest/gtest.h>

#include <headers/variantx.hpp>
#include <print>
#include <type_traits>
#include <utility>
#include <variant>

#include "test-utilities/counter.hpp"

TEST(Variantx, StaticAssertions)
{
    namespace vx = variantx;

    using V1            = vx::Variant<int, float, double, char>;
    using IndexSequence = std::make_index_sequence<4>;

    std::invoke(
        []<std::size_t... Indices, typename... Args>(
            [[maybe_unused]] std::index_sequence<Indices...>&&,
            [[maybe_unused]] vx::Variant<Args...>&&)
        {
            static_assert((
                (std::is_same_v<Args, vx::VariantAlternativeType<Indices, vx::Variant<Args...>>>) &&
                ...));

            static_assert((
                (std::is_same_v<const Args,
                                vx::VariantAlternativeType<Indices, const vx::Variant<Args...>>>) &&
                ...));
        },
        IndexSequence{}, V1{0});  // TODO: default ctor
}

TEST(Variantx, GetIf)
{
    namespace vx = variantx;
    namespace tu = test_utilities;

    std::shared_ptr<tu::detail::CounterBlock> std_block;
    std::shared_ptr<tu::detail::CounterBlock> vx_block;

    {
        tu::Counter std_counter;
        tu::Counter x_counter;

        std::variant<tu::Counter, int, float, double> std_variant(std_counter);
        vx::Variant<tu::Counter, int, float, double>  vx_variant(x_counter);

        tu::Counter* std_c = std::get_if<0>(&std_variant);
        tu::Counter* vx_c  = vx::GetIf<0>(&vx_variant);

        ASSERT_NE(std_c, nullptr);
        ASSERT_NE(vx_c, nullptr);

        std_block = std_c->GetCounterBlock();
        vx_block  = vx_c->GetCounterBlock();
    }

    // TODO: fix dtor
    // std::println("std:\n{}\nmy: {}\n", *std_block, *vx_block);
    // ASSERT_EQ(*std_block, *vx_block);
}
