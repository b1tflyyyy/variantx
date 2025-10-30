#include <print>
#include <type_traits>
#include <utilities.hpp>
#include <variant>
#include <variantx.hpp>

namespace test
{
    static_assert(utilities::GetTypeIndex<int, int, float, double, char> == 0);
    static_assert(utilities::GetTypeIndex<float, int, float, double, char> == 1);
    static_assert(utilities::GetTypeIndex<double, int, float, double, char> == 2);
    static_assert(utilities::GetTypeIndex<char, int, float, double, char> == 3);

    using V1 = variantx::Variant<int, float, double, char>;

    static_assert(std::is_same_v<int, variantx::VariantAlternativeType<0, V1>>);
    static_assert(std::is_same_v<float, variantx::VariantAlternativeType<1, V1>>);
    static_assert(std::is_same_v<double, variantx::VariantAlternativeType<2, V1>>);
    static_assert(std::is_same_v<char, variantx::VariantAlternativeType<3, V1>>);

    static_assert(
        std::is_same_v<const int, variantx::VariantAlternativeType<0, const V1>>);
    static_assert(
        std::is_same_v<const float, variantx::VariantAlternativeType<1, const V1>>);
    static_assert(
        std::is_same_v<const double, variantx::VariantAlternativeType<2, const V1>>);
    static_assert(
        std::is_same_v<const char, variantx::VariantAlternativeType<3, const V1>>);

    using StdAlter = std::variant_alternative_t<0, std::variant<int>>;
    using XAlter = variantx::VariantAlternativeType<0, variantx::Variant<int>>;

    static_assert(std::is_same_v<StdAlter, XAlter>);
}  // namespace test

int main()
{
    // TODO: add google test framework
    std::variant<int> std_variant(1);
    variantx::Variant<int> x_variant(1);

    auto std_check_value = [](auto& variant)
    {
        if (const int* pval = std::get_if<int>(&variant))
        {
            std::println("variant value: {}", *pval);
        }
        else
        {
            std::println("failed to get value!");
        }
    };

    auto x_check_value = [](auto& variant)
    {
        if (const int* pval = variantx::GetIf<int>(&variant))
        {
            std::println("variant value: {}", *pval);
        }
        else
        {
            std::println("failed to get value!");
        }
    };

    std_check_value(std_variant);
    x_check_value(x_variant);

    return 0;
}
