#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <new>
#include <type_traits>
#include <utilities.hpp>
#include <variantx-exceptions.hpp>

namespace variantx
{
    using IndexType = std::size_t;

    // NOLINTNEXTLINE
    inline constexpr IndexType variant_npos = std::numeric_limits<IndexType>::max();

    namespace impl
    {
        // clang-format off
        #define VARIANTX_EAT_SEMICOLON static_assert(true, "")
        // clang-format on

        // Order and value matters
        enum class Trait : std::uint8_t
        {
            TriviallyAvailable = 0,
            Available          = 1,
            Unavailable        = 2
        };

        // clang-format off
        template <typename T,
                  template <typename> typename IsTriviallyAvailable,
                  template <typename> typename IsAvailable>
        constexpr Trait kTrait = IsTriviallyAvailable<T>::value ? Trait::TriviallyAvailable
                                 : IsAvailable<T>::value ? Trait::Available // NOLINT
                                 : Trait::Unavailable;
        // clang-format on

        constexpr Trait CommonTrait(std::initializer_list<Trait> traits)
        {
            return std::max(traits);
        }

        template <typename... Ts>
        struct Traits
        {
            static constexpr Trait kCopyConstructibleTrait = CommonTrait(
                {kTrait<Ts, std::is_trivially_copy_constructible, std::is_copy_constructible>...});

            static constexpr Trait kMoveConstructibleTrait = CommonTrait(
                {kTrait<Ts, std::is_trivially_move_constructible, std::is_move_constructible>...});

            static constexpr Trait kCopyAssignableTrait = CommonTrait(
                {kTrait<Ts, std::is_trivially_copy_assignable, std::is_copy_assignable>...});

            static constexpr Trait kMoveAssignableTrait = CommonTrait(
                {kTrait<Ts, std::is_trivially_move_assignable, std::is_move_assignable>...});

            static constexpr Trait kDestructibleTrait =
                CommonTrait({kTrait<Ts, std::is_trivially_destructible, std::is_destructible>...});
        };

        namespace access
        {
            struct VariadicUnion
            {
                template <typename TVariadicUnion>
                static constexpr auto&& GetAlternative(
                    TVariadicUnion&& variadic_union,
                    [[maybe_unused]] std::in_place_index_t<0>)  // NOLINT -> unnamed parameter
                {
                    return std::forward<TVariadicUnion>(variadic_union).head_;
                }

                template <typename TVariadicUnion, std::size_t Index>
                static constexpr auto&& GetAlternative(
                    TVariadicUnion&& variadic_union,
                    [[maybe_unused]] std::in_place_index_t<Index>)  // NOLINT -> unnamed parameter
                {
                    return GetAlternative(std::forward<TVariadicUnion>(variadic_union).tail_,
                                          std::in_place_index_t<Index - 1>());
                }
            };

            struct Base
            {
                template <std::size_t Index, typename TBase>
                static constexpr auto&& GetAlternative(TBase&& base)
                {
                    return VariadicUnion::GetAlternative(std::forward<TBase>(base).variadic_union_,
                                                         std::in_place_index_t<Index>());
                }
            };

            // TODO:
            struct Variantx
            {
                /*
                template <std::size_t Index, typename TVariantx>
                static constexpr auto&& GetAlternative(TVariantx&& variantx)
                {
                    return Base::GetAlternative(...)
                }
                */
            };
        }  // namespace access

        namespace visitation
        {
            // TODO:
        }

        template <Trait TraitType, std::size_t Index, typename... Ts>
        union VariadicUnion;

        template <Trait TraitType, std::size_t Index>
        union VariadicUnion<TraitType, Index>
        {
        };

        // clang-format off
        // NOLINTNEXTLINE -> use constexpr instead of macros
        #define VARIANTX_VARIADIC_UNION(destructible_trait, destructor_definition)                              \
        template <std::size_t Index, typename Head, typename... Tail>                                           \
        union VariadicUnion<destructible_trait, Index, Head, Tail...>                                           \
        {                                                                                                       \
            friend struct access::VariadicUnion;                                                                \
                                                                                                                \
        public:                                                                                                 \
            template <typename... Args>                                                                         \
            constexpr explicit VariadicUnion([[maybe_unused]] std::in_place_index_t<0>, Args&&... args)         \
                : head_(std::forward<Args>(args)...)                                                            \
            {                                                                                                   \
            }                                                                                                   \
                                                                                                                \
            template <std::size_t IpIndex, typename... Args>                                                    \
            constexpr explicit VariadicUnion([[maybe_unused]] std::in_place_index_t<IpIndex>, Args&&... args)   \
                : tail_(std::in_place_index_t<IpIndex - 1>(), std::forward<Args>(args)...)                      \
            {                                                                                                   \
            }                                                                                                   \
                                                                                                                \
            VariadicUnion(const VariadicUnion&) = default;                                                      \
            VariadicUnion(VariadicUnion&&) noexcept = default;                                                  \
            VariadicUnion& operator=(const VariadicUnion&) = default;                                           \
            VariadicUnion& operator=(VariadicUnion&&) noexcept = default;                                       \
                                                                                                                \
            destructor_definition;                                                                              \
                                                                                                                \
                                                                                                                \
        private:                                                                                                \
            Head head_;                                                                                         \
            VariadicUnion<destructible_trait, Index + 1, Tail...> tail_;                                        \
        }
        // clang-format on

        // Generating Trivially Destructible VariadicUnion version
        VARIANTX_VARIADIC_UNION(Trait::TriviallyAvailable,
                                constexpr ~VariadicUnion() noexcept = default);

        // Generating Non-Trivially Destructible VariadicUnion version
        VARIANTX_VARIADIC_UNION(
            Trait::Available, constexpr ~VariadicUnion() noexcept {} VARIANTX_EAT_SEMICOLON);

        // Generating Non-Destructible VariadicUnion version
        VARIANTX_VARIADIC_UNION(Trait::Unavailable, constexpr ~VariadicUnion() noexcept = delete);

        // clang-format off
        #undef VARIANTX_VARIADIC_UNION
        // clang-format on

        template <Trait Trait, typename... Ts>
        class Base
        {
            friend struct access::Base;
            // TODO:
            // friend struct visitation::Base;

        public:
            template <std::size_t Index, typename... Args>
            // NOLINTNEXTLINE -> unnamed parameter
            explicit constexpr Base([[maybe_unused]] std::in_place_index_t<Index>, Args&&... args)
                : index_(Index),
                  variadic_union_(std::in_place_index_t<Index>(), std::forward<Args>(args)...)
            {
            }

            constexpr bool ValuelessByException() const noexcept { return Index() == variant_npos; }

            constexpr IndexType Index() const noexcept { return index_; }

        protected:  // NOLINT -> for readability
            template <typename Self>
            constexpr auto&& AsBase(this Self&& self)
            {
                return std::forward<Self>(self);
            }

            static constexpr std::size_t Size() noexcept { return sizeof...(Ts); }

        protected:  // NOLINT -> for readability
            IndexType                      index_;
            VariadicUnion<Trait, 0, Ts...> variadic_union_;
        };
    }  // namespace impl

    template <std::size_t Index, typename T>
    struct VariantAlternative;

    template <std::size_t Index, typename T>
    using VariantAlternativeType = typename VariantAlternative<Index, T>::Type;

    template <typename... Ts>
    class Variant
    {
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
}  // namespace variantx
