#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <fwd/variantx.hpp>
#include <initializer_list>
#include <type_traits>
#include <utilities.hpp>
#include <variantx-exceptions.hpp>

namespace variantx
{
    namespace impl
    {
        template <typename... Ts>
        constexpr Variant<Ts...>& AsVariant(Variant<Ts...>& variant) noexcept
        {
            return variant;
        }

        template <typename... Ts>
        constexpr const Variant<Ts...>& AsVariant(const Variant<Ts...>& variant) noexcept
        {
            // NOLINTNEXTLINE
            return variant;  // -> possible use-after-free if element was prvalue
        }

        template <typename... Ts>
        constexpr Variant<Ts...>&& AsVariant(Variant<Ts...>&& variant) noexcept
        {
            return std::move(variant);
        }

        template <typename... Ts>
        constexpr const Variant<Ts...>&& AsVariant(const Variant<Ts...>&& variant) noexcept
        {
            return std::move(variant);
        }

        // Light N-dimensional array of function pointers. Used in place of std::array to avoid
        // adding a dependency.
        template <typename T, std::size_t Size>
        struct FArray
        {
            static_assert(Size > 0, "N-dimensional array should never be empty in std::visit");
            T buffer_[Size] = {};  // NOLINT -> c-style array

            constexpr const T& operator[](std::size_t index) const noexcept
            {
                return buffer_[index];
            }
        };

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
            /*
             * Find the most strict trait.
             */
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
            struct Base
            {
            public:
                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitAlternativeAt(std::size_t index,
                                                                   Visitor&&   visitor,
                                                                   Variants&&... variants)
                {
                    constexpr auto kFdiagonal =
                        MakeFDiagonal<Visitor&&,
                                      decltype(std::forward<Variants>(variants).AsBase())...>();

                    return kFdiagonal[index](std::forward<Visitor>(visitor),
                                             std::forward<Variants>(variants).AsBase()...);
                }

                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitAlternative(Visitor&& visitor,
                                                                 Variants&&... variants)
                {
                    constexpr auto kFmatrix =
                        MakeFMatrix<Visitor&&,
                                    decltype(std::forward<Variants>(variants).AsBase())...>();

                    return At(kFmatrix, variants.Index()...)(
                        std::forward<Visitor>(visitor),
                        std::forward<Variants>(variants).AsBase()...);
                }

            private:
                template <typename T>
                static constexpr const T& At(const T& element)
                {
                    // NOLINTNEXTLINE
                    return element;  // -> possible use-after-free if element was prvalue
                }

                template <typename T, std::size_t N, typename... Indices>
                static constexpr auto&& At(const FArray<T, N>& farray, std::size_t index,
                                           Indices... indices)
                {
                    return At(farray[index], indices...);
                }

                template <typename Func, typename... Funcs>
                static constexpr void VisitVisitorReturnTypeCheck()
                {
                    /*
                     * Func && Funcs... should have the same return type.
                     */
                    static_assert(
                        std::conjunction_v<std::is_same<Func, Funcs>...>,
                        "`variantx::visit` requires the visitor to have a single return type.");
                }

                template <typename... Funcs>
                static constexpr auto MakeFArray(Funcs&&... funcs)
                {
                    // Check.
                    VisitVisitorReturnTypeCheck<std::remove_cvref_t<Funcs>...>();

                    /*
                     * Return FArray with common type for all funcs && filled with funcs...
                     */
                    using Result =
                        FArray<std::common_type_t<std::remove_cvref_t<Funcs>...>, sizeof...(Funcs)>;
                    return Result{{std::forward<Funcs>(funcs)...}};
                }

                template <std::size_t... Indices>
                struct Dispatcher
                {
                    template <typename Func, typename... Variants>
                    static constexpr decltype(auto) Dispatch(Func func, Variants... variants)
                    {
                        /*
                         * Invoke specific func for specific index in variant.
                         *
                         * Example:
                         * Indices... == 1, 2, 3
                         * Variants... == v1, v2, v3
                         *
                         * std::invoke(func, get<1>(v1), get<2>(v2), get<3>(v3))
                         */

                        // NOLINTBEGIN
                        return std::invoke(static_cast<Func>(func),
                                           access::Base::GetAlternative<Indices>(
                                               static_cast<Variants>(variants))...);
                        // NOLINTEND
                    }
                };

                template <typename Func, typename... Variants, std::size_t... Indices>
                // NOLINTNEXTLINE -> unnamed parameter
                static constexpr auto MakeDispatch([[maybe_unused]] std::index_sequence<Indices...>)
                {
                    /*
                     * Just wrapper around Dispatcher<...>::Dispatch<...>
                     * Returns type of Dispatcher<...>::Dispatch<...> (function pointer)
                     */
                    return Dispatcher<Indices...>::template Dispatch<Func, Variants...>;
                }

                template <std::size_t Index, typename Func, typename... Variants>
                static constexpr auto MakeFDiagonalImpl()
                {
                    /*
                     * std::index_sequence<((void)std::type_identity<Variants>(), Index)...>()
                     *
                     * pattern:
                     * repeating the same Index sizeof...(Variants) times
                     *
                     * example:
                     * sizeof...(Variants) == 3
                     * Index == 1
                     *
                     * std::index_sequence<1, 1, 1>
                     */
                    return MakeDispatch<Func, Variants...>(
                        std::index_sequence<((void)std::type_identity<Variants>(), Index)...>());
                }

                template <typename Func, typename... Variants, std::size_t... Indices>
                static constexpr auto MakeFDiagonalImpl(
                    [[maybe_unused]] std::index_sequence<Indices...>)  // NOLINT-> unnamed parameter
                {
                    /*
                     * Make Function Array for every index in Variants...
                     *
                     * Example:
                     * Indices... == 1, 2, 3
                     * Variants... == v1, v2
                     *
                     * Base::MakeFArray(foo<1, Func, v1, v2>(), foo<2, Func, v1, v2>())
                     */
                    return Base::MakeFArray(MakeFDiagonalImpl<Indices, Func, Variants...>()...);
                }

                template <typename Func, typename Variant, typename... Variants>
                static constexpr auto MakeFDiagonal()
                {
                    // Basic size check, all of variants should have the same size
                    constexpr std::size_t kSize = std::remove_cvref_t<Variant>::Size();
                    static_assert(((kSize == std::remove_cvref_t<Variants>::Size()) && ...));

                    /*
                     * Wrapper around MakeFDiagonalImpl that provides Indices... ->
                     * std::make_index_sequence<kSize>()
                     */
                    return MakeFDiagonalImpl<Func, Variant, Variants...>(
                        std::make_index_sequence<kSize>());
                }

                template <typename Func, typename... Variants, std::size_t... Indices>
                static constexpr auto MakeFMatrixImpl(std::index_sequence<Indices...> indices)
                {
                    return MakeDispatch<Func, Variants...>(indices);
                }

                template <typename Func, typename... Variants, std::size_t... Is, std::size_t... Js,
                          typename... Ls>
                static constexpr auto MakeFMatrixImpl(
                    [[maybe_unused]] std::index_sequence<Is...>,            // NOLINT
                    [[maybe_unused]] std::index_sequence<Js...>, Ls... ls)  // NOLINT
                {
                    /*
                     * Recursive Matrix construction.
                     *
                     * Call 1:
                     * Func, Variants... (v1, v2), Is... == void, Js... == <0, 1>, Ls... == <0, 1>
                     * return Base::MakeFArray(MakeFMatrixImpl<Func, Variants...>(<0>(), <0, 1>,
                     *                                                            <1>(), <0, 1>)
                     *
                     * Call 2:
                     *
                     * 2.1: part [<0>(), <0, 1>]
                     * Func, Variants..., Is... == 0, Js... == <0, 1>, Ls... == void
                     * return Base::MakeFArray(MakeFMatrixImpl<Func, Variants...>(<0, 0>(), <void>,
                     *                                                            <0, 1>(), <void>)
                     * <0, 0>(), <void> -> goes to MakeFMatrixImpl specialization with return
                     * MakeDispatch<Func, Variants...>(indices);
                     *
                     * <0, 1>(), <void> -> <void> -> goes to MakeFMatrixImpl specialization with
                     * MakeDispatch<Func, Variants...>(indices);
                     *
                     * 2.2: part [<1>(), <0, 1>]
                     * Func, Variants..., Is... == 1, Js... == <0, 1>, Ls... == void
                     * return Base::MakeFArray(MakeFMatrixImpl<Func, Variants...>(<1, 0>(), <void>,
                     *                                                            <1, 1>(), <void>)
                     *
                     * <1, 0>(), <void> -> goes to MakeFMatrixImpl specialization with return
                     * MakeDispatch<Func, Variants...>(indices);
                     *
                     * <1, 1>(), <void> -> goes to MakeFMatrixImpl specialization with return
                     * MakeDispatch<Func, Variants...>(indices);
                     */
                    return Base::MakeFArray(MakeFMatrixImpl<Func, Variants...>(
                        std::index_sequence<Is..., Js>(), ls...)...);
                }

                template <typename Func, typename... Variants>
                static constexpr auto MakeFMatrix()
                {
                    /*
                     * Wrapper around MakeFMatrixImpl.
                     * Example:
                     * Variants... == v1, v2
                     * sizeof(v1) == sizeof(v2) == 2
                     *
                     * return MakeFMatrixImpl<Func, Variants...>(
                         std::index_sequence<>(),
                         std::make_index_sequence<2>(),
                         std::make_index_sequence<2>());
                     */
                    return MakeFMatrixImpl<Func, Variants...>(
                        std::index_sequence<>(),
                        std::make_index_sequence<std::remove_cvref_t<Variants>::Size()>()...);
                }
            };

            struct Variant
            {
                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitAlternativeAt(std::size_t index,
                                                                   Visitor&&   visitor,
                                                                   Variants&&... variants)
                {
                    return Base::VisitAlternativeAt(index, std::forward<Visitor>(visitor),
                                                    std::forward<Variants>(variants).impl_...);
                }

                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitAlternative(Visitor&& visitor,
                                                                 Variants... variants)
                {
                    return Base::VisitAlternative(
                        std::forward<Visitor>(visitor),
                        AsVariant(std::forward<Variants>(variants)).impl_...);
                }
            };
        }  // namespace visitation

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
            friend struct visitation::Base;

        public:
            template <std::size_t Index, typename... Args>
            // NOLINTNEXTLINE -> unnamed parameter
            explicit constexpr Base([[maybe_unused]] std::in_place_index_t<Index>, Args&&... args)
                : index_(Index),
                  variadic_union_(std::in_place_index_t<Index>(), std::forward<Args>(args)...)
            {
            }

            constexpr bool ValuelessByException() const noexcept { return Index() == kVariantNpos; }

            constexpr std::size_t Index() const noexcept { return index_; }

        protected:  // NOLINT -> for readability
            template <typename Self>
            constexpr auto&& AsBase(this Self&& self)
            {
                return std::forward<Self>(self);
            }

            static constexpr std::size_t Size() noexcept { return sizeof...(Ts); }

        protected:  // NOLINT -> for readability
            std::size_t                    index_;
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
