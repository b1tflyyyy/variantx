#include <gtest/gtest.h>

#include <headers/variantx.hpp>
#include <type_traits>
#include <utility>
#include <variant>

#include "test-utilities/counter.hpp"
#include "test-utilities/throw.hpp"

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
        IndexSequence{}, V1{});
}

TEST(Variantx, ThrowOnCopy)
{
    namespace vx = variantx;
    namespace tu = test_utilities;

    vx::Variant<int, tu::ThrowOnCopy> x_variant;

    ASSERT_NE(vx::GetIf<0>(&x_variant), nullptr);
    ASSERT_NE(vx::GetIf<int>(&x_variant), nullptr);

    ASSERT_EQ(*vx::GetIf<0>(&x_variant), 0);
    ASSERT_EQ(*vx::GetIf<int>(&x_variant), 0);

    ASSERT_EQ(vx::GetIf<1>(&x_variant), nullptr);
    ASSERT_EQ(vx::GetIf<tu::ThrowOnCopy>(&x_variant), nullptr);

    // NOLINTBEGIN
    try
    {
        x_variant.Emplace<tu::ThrowOnCopy>(tu::ThrowOnCopy{22});
        FAIL() << "exception expected!";
    }
    catch (...)
    {
    }
    // NOLINTEND

    ASSERT_TRUE(x_variant.ValuelessByException());
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

    ASSERT_EQ(*std_block, *vx_block);
}

TEST(Variantx, Get)
{
    namespace vx = variantx;
    namespace tu = test_utilities;

    std::shared_ptr<tu::detail::CounterBlock> std_block;
    std::shared_ptr<tu::detail::CounterBlock> vx_block;

    {
        tu::Counter std_counter;
        tu::Counter x_counter;

        tu::Counter const_std_counter;
        tu::Counter const_x_counter;

        std::variant<tu::Counter, int, float, double> std_variant(std_counter);
        vx::Variant<tu::Counter, int, float, double>  vx_variant(x_counter);

        const std::variant<tu::Counter, int, float, double> const_std_variant(const_std_counter);
        const vx::Variant<tu::Counter, int, float, double>  const_vx_variant(const_x_counter);

        static_assert(std::is_same_v<tu::Counter&, decltype(vx::Get<0>(vx_variant))>);
        static_assert(std::is_same_v<const tu::Counter&, decltype(vx::Get<0>(const_vx_variant))>);

        static_assert(std::is_same_v<tu::Counter&&, decltype(vx::Get<0>(std::move(vx_variant)))>);
        static_assert(
            std::is_same_v<const tu::Counter&&, decltype(vx::Get<0>(std::move(const_vx_variant)))>);

        /* std::get, vx::Get by index */
        std_block = std::get<0>(std_variant).GetCounterBlock();
        vx_block  = vx::Get<0>(vx_variant).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);

        std_block = std::get<0>(std::move(std_variant)).GetCounterBlock();
        vx_block  = vx::Get<0>(std::move(vx_variant)).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);

        std_block = std::get<0>(const_std_variant).GetCounterBlock();
        vx_block  = vx::Get<0>(const_vx_variant).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);

        // NOLINTBEGIN
        std_block = std::get<0>(std::move(const_std_variant)).GetCounterBlock();
        vx_block  = vx::Get<0>(std::move(const_vx_variant)).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);
        // NOLINTEND

        /* std::get, vx::Get by type */
        std_block = std::get<tu::Counter>(std_variant).GetCounterBlock();
        vx_block  = vx::Get<tu::Counter>(vx_variant).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);

        std_block = std::get<tu::Counter>(std::move(std_variant)).GetCounterBlock();
        vx_block  = vx::Get<tu::Counter>(std::move(vx_variant)).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);

        std_block = std::get<tu::Counter>(const_std_variant).GetCounterBlock();
        vx_block  = vx::Get<tu::Counter>(const_vx_variant).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);

        // NOLINTBEGIN
        std_block = std::get<tu::Counter>(std::move(const_std_variant)).GetCounterBlock();
        vx_block  = vx::Get<tu::Counter>(std::move(const_vx_variant)).GetCounterBlock();
        ASSERT_EQ(*std_block, *vx_block);
        // NOLINTEND
    }

    ASSERT_EQ(*std_block, *vx_block);
}

TEST(Variantx, EmplaceIndex)
{
    namespace vx = variantx;
    namespace tu = test_utilities;

    std::shared_ptr<tu::detail::CounterBlock> std_block_outer;
    std::shared_ptr<tu::detail::CounterBlock> vx_block_outer;

    std::shared_ptr<tu::detail::CounterBlock> std_block_emplaced;
    std::shared_ptr<tu::detail::CounterBlock> vx_block_emplaced;

    {
        tu::Counter std_counter;
        tu::Counter x_counter;

        std_block_outer = std_counter.GetCounterBlock();
        vx_block_outer  = x_counter.GetCounterBlock();

        std::variant<tu::Counter, int, float> std_variant(std_counter);
        vx::Variant<tu::Counter, int, float>  vx_variant(x_counter);

        ASSERT_EQ(std_variant.index(), 0);
        ASSERT_EQ(vx_variant.Index(), 0);
        ASSERT_EQ(*std_block_outer, *vx_block_outer);

        std_variant.emplace<1>(42);  // NOLINT
        vx_variant.Emplace<1>(42);   // NOLINT

        ASSERT_EQ(std_variant.index(), 1);
        ASSERT_EQ(vx_variant.Index(), 1);
        ASSERT_EQ(*std::get_if<1>(&std_variant), 42);
        ASSERT_EQ(*vx::GetIf<1>(&vx_variant), 42);
        ASSERT_EQ(*std_block_outer, *vx_block_outer);

        tu::Counter& std_c_new = std_variant.emplace<0>();
        tu::Counter& vx_c_new  = vx_variant.Emplace<0>();

        std_block_emplaced = std_c_new.GetCounterBlock();
        vx_block_emplaced  = vx_c_new.GetCounterBlock();

        ASSERT_EQ(*std_block_outer, *vx_block_outer);
        ASSERT_EQ(*std_block_emplaced, *vx_block_emplaced);

        std_variant.emplace<2>(3.14f);  // NOLINT
        vx_variant.Emplace<2>(3.14f);   // NOLINT

        ASSERT_EQ(std_variant.index(), 2);
        ASSERT_EQ(vx_variant.Index(), 2);
    }

    ASSERT_EQ(*std_block_outer, *vx_block_outer);
    ASSERT_EQ(*std_block_emplaced, *vx_block_emplaced);
}

TEST(Variantx, EmplaceType)
{
    namespace vx = variantx;
    namespace tu = test_utilities;

    std::shared_ptr<tu::detail::CounterBlock> std_block_outer;
    std::shared_ptr<tu::detail::CounterBlock> vx_block_outer;

    std::shared_ptr<tu::detail::CounterBlock> std_block_emplaced;
    std::shared_ptr<tu::detail::CounterBlock> vx_block_emplaced;

    {
        tu::Counter std_counter;
        tu::Counter x_counter;

        std_block_outer = std_counter.GetCounterBlock();
        vx_block_outer  = x_counter.GetCounterBlock();

        std::variant<tu::Counter, int, float> std_variant(std_counter);
        vx::Variant<tu::Counter, int, float>  vx_variant(x_counter);

        ASSERT_EQ(std_variant.index(), 0);
        ASSERT_EQ(vx_variant.Index(), 0);
        ASSERT_EQ(*std_block_outer, *vx_block_outer);

        std_variant.emplace<int>(42);  // NOLINT
        vx_variant.Emplace<int>(42);   // NOLINT

        ASSERT_EQ(std_variant.index(), 1);
        ASSERT_EQ(vx_variant.Index(), 1);
        ASSERT_EQ(*std::get_if<int>(&std_variant), 42);
        ASSERT_EQ(*vx::GetIf<int>(&vx_variant), 42);
        ASSERT_EQ(*std_block_outer, *vx_block_outer);

        tu::Counter& std_c_new = std_variant.emplace<tu::Counter>();
        tu::Counter& vx_c_new  = vx_variant.Emplace<tu::Counter>();

        std_block_emplaced = std_c_new.GetCounterBlock();
        vx_block_emplaced  = vx_c_new.GetCounterBlock();

        ASSERT_EQ(*std_block_outer, *vx_block_outer);
        ASSERT_EQ(*std_block_emplaced, *vx_block_emplaced);

        std_variant.emplace<float>(3.14f);  // NOLINT
        vx_variant.Emplace<float>(3.14f);   // NOLINT

        ASSERT_EQ(std_variant.index(), 2);
        ASSERT_EQ(vx_variant.Index(), 2);
    }

    ASSERT_EQ(*std_block_outer, *vx_block_outer);
    ASSERT_EQ(*std_block_emplaced, *vx_block_emplaced);
}

TEST(Variantx, GetThrowsBadVariantAccess)
{
    namespace vx = variantx;

    vx::Variant<int, float, std::string> vx_variant(42);  // NOLINT

    ASSERT_EQ(vx_variant.Index(), 0);

    try
    {
        ASSERT_EQ(vx::Get<int>(vx_variant), 42);
        ASSERT_EQ(vx::Get<0>(vx_variant), 42);
    }
    catch (...)
    {
        FAIL() << "Get threw an exception when it shouldn't have!";
    }

    try
    {
        [[maybe_unused]] auto& val = vx::Get<1>(vx_variant);
        FAIL() << "exception expected!";
    }
    catch (const vx::BadVariantAccess& e)
    {
        ASSERT_STREQ(e.what(), "Variant does not hold this value");
    }
    catch (...)
    {
        FAIL() << "Caught wrong exception type (expected BadVariantAccess)!";
    }

    try
    {
        [[maybe_unused]] auto& val = vx::Get<std::string>(vx_variant);
        FAIL() << "exception expected!";
    }
    catch (const vx::BadVariantAccess& e)
    {
        ASSERT_STREQ(e.what(), "Variant does not hold this value");
    }
    catch (...)
    {
        FAIL() << "Caught wrong exception type (expected BadVariantAccess)!";
    }
}

TEST(Variantx, IndexConstNonConst)
{
    namespace vx = variantx;

    using V = vx::Variant<int, float, std::string>;

    V non_const_variant(10);  // NOLINT

    static_assert(std::is_same_v<decltype(non_const_variant.Index()), std::size_t>);
    ASSERT_EQ(non_const_variant.Index(), 0);

    non_const_variant.Emplace<float>(3.14f);  // NOLINT
    ASSERT_EQ(non_const_variant.Index(), 1);

    const V const_variant(std::string("hello"));

    static_assert(std::is_same_v<decltype(const_variant.Index()), std::size_t>);
    ASSERT_EQ(const_variant.Index(), 2);

    /* TODO: copy ctor
    const V const_variant_copy = non_const_variant;
    ASSERT_EQ(const_variant_copy.Index(), 1);
    */
}
