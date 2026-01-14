#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <fwd/variantx.hpp>
#include <initializer_list>
#include <sfinae_helperx.hpp>
#include <type_traits>
#include <utilities.hpp>
#include <variantx-exceptions.hpp>

namespace variantx
{
    template <typename T>
    struct VariantSize<const T> : VariantSize<T>
    {
    };

    template <typename T>
    struct VariantSize<volatile T>
    {
        static_assert(false,
                      "Deprecated since C++20 "
                      "https://en.cppreference.com/w/cpp/utility/variant/variant_alternative.html");
    };

    template <typename T>
    struct VariantSize<const volatile T>
    {
        static_assert(false,
                      "Deprecated since C++20 "
                      "https://en.cppreference.com/w/cpp/utility/variant/variant_alternative.html");
    };

    template <typename... Ts>
    struct VariantSize<Variant<Ts...>>
    {
        static constexpr std::size_t kValue = sizeof...(Ts);
    };

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

        struct ValuelessTag
        {
        };

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
                {kCopyConstructibleTrait,
                 kTrait<Ts, std::is_trivially_copy_assignable, std::is_copy_assignable>...});

            static constexpr Trait kMoveAssignableTrait = CommonTrait(
                {kMoveConstructibleTrait,
                 kTrait<Ts, std::is_trivially_move_assignable, std::is_move_assignable>...});

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

            struct Variant
            {
                template <std::size_t Index, typename TVariant>
                static constexpr auto&& GetAlternative(TVariant&& variant)
                {
                    return Base::GetAlternative<Index>(std::forward<TVariant>(variant).impl_);
                }
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
                        "`variantx::Visit` requires the visitor to have a single return type.");
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
            public:
                // Internal visitor
                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitAlternativeAt(std::size_t index,
                                                                   Visitor&&   visitor,
                                                                   Variants&&... variants)
                {
                    return Base::VisitAlternativeAt(index, std::forward<Visitor>(visitor),
                                                    std::forward<Variants>(variants).impl_...);
                }

                // Internal visitor
                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitAlternative(Visitor&& visitor,
                                                                 Variants&&... variants)
                {
                    return Base::VisitAlternative(
                        std::forward<Visitor>(visitor),
                        AsVariant(std::forward<Variants>(variants)).impl_...);
                }

                // External visitor
                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitValueAt(std::size_t index, Visitor&& visitor,
                                                             Variants&&... variants)
                {
                    return VisitAlternativeAt(index,
                                              MakeValueVisitor(std::forward<Visitor>(visitor)),
                                              std::forward<Variants>(variants)...);
                }

                template <typename Visitor, typename... Variants>
                static constexpr decltype(auto) VisitValue(Visitor&& visitor,
                                                           Variants&&... variants)
                {
                    return VisitAlternative(MakeValueVisitor(std::forward<Visitor>(visitor)),
                                            std::forward<Variants>(variants)...);
                }

                template <typename Ret, typename Visitor, typename... Variants>
                static constexpr Ret VisitValue(Visitor&& visitor, Variants&&... variants)
                {
                    return VisitAlternative(MakeValueVisitor<Ret>(std::forward<Visitor>(visitor)),
                                            std::forward<Variants>(variants)...);
                }

            private:
                template <typename Visitor, typename... Values>
                static constexpr void VisitExhaustiveVisitorCheck()
                {
                    static_assert(std::is_invocable_v<Visitor, Values...>,
                                  "`variantx::Visit` requires the visitor to be exhaustive.");
                }

                template <typename Visitor>
                struct ValueVisitor
                {
                    template <typename... Alternatives>
                    constexpr decltype(auto) operator()(Alternatives&&... alternatives) const
                    {
                        VisitExhaustiveVisitorCheck<
                            Visitor,
                            decltype((std::forward<Alternatives>(alternatives).value_))...>();

                        return std::invoke(std::forward<Visitor>(visitor_),
                                           std::forward<Alternatives>(alternatives).value_...);
                    }

                    Visitor&& visitor_;  // NOLINT -> ref data member
                };

                template <typename Ret, typename Visitor>
                struct ValueVisitorReturnType
                {
                    template <typename... Alternatives>
                    constexpr Ret operator()(Alternatives&&... alternatives) const
                    {
                        VisitExhaustiveVisitorCheck<
                            Visitor,
                            decltype((std::forward<Alternatives>(alternatives).value_))...>();

                        if constexpr (std::is_void_v<Ret>)
                        {
                            std::invoke(std::forward<Visitor>(visitor_),
                                        std::forward<Alternatives>(alternatives).value_...);
                        }
                        else
                        {
                            return std::invoke(std::forward<Visitor>(visitor_),
                                               std::forward<Alternatives>(alternatives).value_...);
                        }
                    }

                    Visitor&& visitor_;  // NOLINT -> ref data member
                };

                template <typename Visitor>
                static constexpr auto MakeValueVisitor(Visitor&& visitor)
                {
                    return ValueVisitor<Visitor>{std::forward<Visitor>(visitor)};
                }

                template <typename Ret, typename Visitor>
                static constexpr auto MakeValueVisitor(Visitor&& visitor)
                {
                    return ValueVisitorReturnType<Ret, Visitor>{std::forward<Visitor>(visitor)};
                }
            };
        }  // namespace visitation

        template <std::size_t Index, typename T>
        struct Alternative
        {
            using ValueType                     = T;
            static constexpr std::size_t kIndex = Index;

            template <typename... Args>
            // NOLINTNEXTLINE -> unnamed parameter
            explicit constexpr Alternative(std::in_place_t, Args&&... args)
                : value_(std::forward<Args>(args)...)
            {
            }

            ValueType value_;
        };

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
            constexpr explicit VariadicUnion([[maybe_unused]] ValuelessTag) noexcept : dummy_()                 \
            {                                                                                                   \
            }                                                                                                   \
                                                                                                                \
            template <typename... Args>                                                                         \
            constexpr explicit VariadicUnion([[maybe_unused]] std::in_place_index_t<0>, Args&&... args)         \
                : head_(std::in_place_t(), std::forward<Args>(args)...)                                         \
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
            VariadicUnion(VariadicUnion&&)  = default;                                                          \
            VariadicUnion& operator=(const VariadicUnion&) = default;                                           \
            VariadicUnion& operator=(VariadicUnion&&)  = default;                                               \
                                                                                                                \
            destructor_definition;                                                                              \
                                                                                                                \
                                                                                                                \
        private:                                                                                                \
            char dummy_;                                                                                        \
                                                                                                                \
            Alternative<Index, Head> head_;                                                                     \
            VariadicUnion<destructible_trait, Index + 1, Tail...> tail_;                                        \
        }
        // clang-format on

        // Generating Trivially Destructible VariadicUnion version
        VARIANTX_VARIADIC_UNION(Trait::TriviallyAvailable, constexpr ~VariadicUnion() = default);

        // Generating Non-Trivially Destructible VariadicUnion version
        VARIANTX_VARIADIC_UNION(
            Trait::Available, constexpr ~VariadicUnion() {} VARIANTX_EAT_SEMICOLON);

        // Generating Non-Destructible VariadicUnion version
        VARIANTX_VARIADIC_UNION(Trait::Unavailable, constexpr ~VariadicUnion() = delete);

        // clang-format off
        #undef VARIANTX_VARIADIC_UNION
        // clang-format on

        template <Trait Trait, typename... Ts>
        class Base
        {
            friend struct access::Base;
            friend struct visitation::Base;

        public:
            using IndexType = std::size_t;

            explicit constexpr Base(ValuelessTag tag) : index_(kVariantNpos), variadic_union_(tag)
            {
            }

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

        protected:                                           // NOLINT -> for readability
            std::size_t                    index_;           // NOLINT
            VariadicUnion<Trait, 0, Ts...> variadic_union_;  // NOLINT
        };

        template <typename Traits, Trait = Traits::kDestructibleTrait>
        class Dtor;

        // clang-format off
        // NOLINTNEXTLINE -> use constexpr instead of macros
        #define VARIANTX_DESTRUCTOR(destructible_trait, destructor_definition, destroy)                                            \
        template <typename... Ts>                                                                                                  \
        class Dtor<Traits<Ts...>, destructible_trait> : public Base<destructible_trait, Ts...>                                     \
        {                                                                                                                          \
            using BaseType = Base<destructible_trait, Ts...>;                                                                      \
            using IndexType = typename BaseType::IndexType;                                                                        \
                                                                                                                                   \
        public:                                                                                                                    \
            using BaseType::BaseType;                                                                                              \
            using BaseType::operator=;                                                                                             \
                                                                                                                                   \
            Dtor(const Dtor&) = default;                                                                                           \
            Dtor(Dtor&&) = default;                                                                                                \
            Dtor& operator=(const Dtor&) = default;                                                                                \
            Dtor& operator=(Dtor&&) = default;                                                                                     \
                                                                                                                                   \
            destructor_definition;                                                                                                 \
                                                                                                                                   \
        protected:                                                                                                                 \
            destroy; /* NOLINT */                                                                                                  \
        }
        // clang-format on

        // clang-format off

        // Generating Trivially Destructible Dtor version
        VARIANTX_DESTRUCTOR(
            Trait::TriviallyAvailable,
            constexpr ~Dtor() = default,
            constexpr void Destroy() noexcept
            {
                this->index_ = kVariantNpos;
            } VARIANTX_EAT_SEMICOLON);

        // Generating Non-Trivially Destructible Dtor version
        VARIANTX_DESTRUCTOR(
            Trait::Available,
            constexpr ~Dtor() { Destroy(); } VARIANTX_EAT_SEMICOLON,
            constexpr void Destroy() noexcept
            {
                if (!this->ValuelessByException())
                {
                    visitation::Base::VisitAlternative([](auto& alternative) noexcept
                    {
                        using Type = std::remove_cvref_t<decltype(alternative)>;
                        alternative.~Type();
                    }, *this);
                }

                this->index_ = kVariantNpos;
            } VARIANTX_EAT_SEMICOLON);

        // Generating Non-Destructible Dtor version
        VARIANTX_DESTRUCTOR(
            Trait::Unavailable,
            constexpr ~Dtor() = delete,
            constexpr void Destroy() noexcept = delete
        );

        #undef VARIANTX_DESTRUCTOR
        // clang-format on

        template <typename Traits>
        class Ctor : public Dtor<Traits>
        {
            using BaseType = Dtor<Traits>;

        public:
            using BaseType::BaseType;
            using BaseType::operator=;

        protected:
            template <typename Rhs>
            static constexpr void GenericConstruct(Ctor& lhs, Rhs&& rhs)
            {
                lhs.Destroy();

                if (!rhs.ValuelessByException())
                {
                    auto rhs_index = rhs.Index();
                    visitation::Base::VisitAlternativeAt(
                        rhs_index,
                        [&lhs](auto&& rhs_alt)
                        {
                            std::construct_at(
                                std::addressof(lhs.variadic_union_),
                                std::in_place_index<std::decay_t<decltype(rhs_alt)>::kIndex>,
                                std::forward<decltype(rhs_alt)>(rhs_alt).value_);
                        },
                        std::forward<Rhs>(rhs));

                    lhs.index_ = rhs_index;
                }
            }
        };

        template <typename Traits, Trait = Traits::kMoveConstructibleTrait>
        class MoveCtor;

        // clang-format off
        // NOLINTNEXTLINE
        #define VARIANTX_MOVE_CTOR(move_constructible_trait, move_constructor_definition)               \
        template <typename... Ts>                                                                       \
        class MoveCtor<Traits<Ts...>, move_constructible_trait> : public Ctor<Traits<Ts...>>            \
        {                                                                                               \
            using BaseType = Ctor<Traits<Ts...>>;                                                       \
                                                                                                        \
        public:                                                                                         \
            using BaseType::BaseType;                                                                   \
            using BaseType::operator=;                                                                  \
                                                                                                        \
            MoveCtor(const MoveCtor&) = default;                                                        \
            ~MoveCtor() = default;                                                                      \
            MoveCtor& operator=(const MoveCtor&) = default;                                             \
            MoveCtor& operator=(MoveCtor&&) = default;                                                  \
                                                                                                        \
            move_constructor_definition;                                                                \
        }

        VARIANTX_MOVE_CTOR(Trait::TriviallyAvailable,
                           constexpr MoveCtor(MoveCtor&& that) = default);

        VARIANTX_MOVE_CTOR(Trait::Available,
                           constexpr MoveCtor(MoveCtor&& that) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>) : MoveCtor(ValuelessTag())
                           {
                                this->GenericConstruct(*this, std::move(that));
                           } VARIANTX_EAT_SEMICOLON);

        VARIANTX_MOVE_CTOR(Trait::Unavailable,
                           constexpr MoveCtor(MoveCtor&&) = delete);

        #undef VARIANTX_MOVE_CTOR

        template <typename Traits, Trait = Traits::kCopyConstructibleTrait>
        class CopyCtor;

        // NOLINTNEXTLINE
        #define VARIANTX_COPY_CTOR(copy_constructible_trait, copy_constructor_definition)           \
        template <typename... Ts>                                                                   \
        class CopyCtor<Traits<Ts...>, copy_constructible_trait> : public MoveCtor<Traits<Ts...>>    \
        {                                                                                           \
            using BaseType = MoveCtor<Traits<Ts...>>;                                               \
                                                                                                    \
        public:                                                                                     \
            using BaseType::BaseType;                                                               \
            using BaseType::operator=;                                                              \
                                                                                                    \
            CopyCtor(CopyCtor&&)  = default;                                                        \
            ~CopyCtor() = default;                                                                  \
                                                                                                    \
            CopyCtor& operator=(const CopyCtor&) = default;                                         \
            CopyCtor& operator=(CopyCtor&&) = default;                                              \
                                                                                                    \
            copy_constructor_definition;                                                            \
        }

        VARIANTX_COPY_CTOR(Trait::TriviallyAvailable,
                           constexpr CopyCtor(const CopyCtor& that) = default);

        VARIANTX_COPY_CTOR(Trait::Available,
                           constexpr CopyCtor(const CopyCtor& that) : CopyCtor(ValuelessTag())
                           {
                                this->GenericConstruct(*this, that);
                           } VARIANTX_EAT_SEMICOLON);

        VARIANTX_COPY_CTOR(Trait::Unavailable,
                           constexpr CopyCtor(const CopyCtor& that) = delete);

        #undef VARIANTX_COPY_CTOR
        // clang-format on

        template <typename Traits>
        class Assignment : public CopyCtor<Traits>
        {
            using BaseType = CopyCtor<Traits>;

        public:
            using BaseType::BaseType;
            using BaseType::operator=;

            template <std::size_t Index, typename... Args>
            constexpr auto& Emplace(Args&&... args)
            {
                this->Destroy();
                std::construct_at(std::addressof(this->variadic_union_),
                                  std::in_place_index_t<Index>(), std::forward<Args>(args)...);

                this->index_ = Index;
                return access::Base::GetAlternative<Index>(*this).value_;
            }

        protected:
            template <std::size_t Index, typename T, typename Arg>
            constexpr void AssignAlternative(Alternative<Index, T>& alternative, Arg&& arg)
            {
                if (this->Index() == Index)
                {
                    alternative.value_ = std::forward<Arg>(arg);
                }
                else
                {
                    struct
                    {
                        // NOLINTNEXTLINE -> unnamed parameter
                        constexpr void operator()(std::true_type) const
                        {
                            this_->Emplace<Index>(std::forward<Arg>(arg_));
                        }

                        // NOLINTNEXTLINE -> unnamed parameter
                        constexpr void operator()(std::false_type) const
                        {
                            this_->Emplace<Index>(T(std::forward<Arg>(arg_)));
                        }

                        Assignment* this_;  // NOLINT -> public visibility
                        Arg&&       arg_;   // NOLINT -> public visibility
                    } impl{this, std::forward<Arg>(arg)};

                    impl(std::bool_constant<std::is_nothrow_constructible_v<T, Arg> ||
                                            !std::is_nothrow_move_constructible_v<T>>());
                }
            }

            template <typename That>
            constexpr void GenericAssign(That&& that)
            {
                if (this->ValuelessByException() && that.ValuelessByException())
                {
                    // do nothing
                }
                else if (that.ValuelessByException())
                {
                    this->Destroy();
                }
                else
                {
                    visitation::Base::VisitAlternativeAt(
                        that.Index(),
                        [this](auto& this_alternative, auto&& that_alternative)
                        {
                            this->AssignAlternative(
                                this_alternative,
                                std::forward<decltype(that_alternative)>(that_alternative).value_);
                        },
                        *this, std::forward<That>(that));
                }
            }
        };

        template <typename Traits, Trait = Traits::kMoveAssignableTrait>
        class MoveAssignment;

        // clang-format off
        // NOLINTNEXTLINE
        #define VARIANTX_MOVE_ASSIGNMENT(move_assignable_trait, move_assignment_definition)             \
        template <typename... Ts>                                                                       \
        class MoveAssignment<Traits<Ts...>, move_assignable_trait> : public Assignment<Traits<Ts...>>   \
        {                                                                                               \
            using BaseType = Assignment<Traits<Ts...>>;                                                 \
                                                                                                        \
        public:                                                                                         \
            using BaseType::BaseType;                                                                   \
            using BaseType::operator=;                                                                  \
                                                                                                        \
            MoveAssignment(const MoveAssignment&) = default;                                            \
            MoveAssignment(MoveAssignment&&) = default;                                                 \
            ~MoveAssignment() = default;                                                                \
            MoveAssignment& operator=(const MoveAssignment&) = default;                                 \
                                                                                                        \
            move_assignment_definition;                                                                 \
        }

        VARIANTX_MOVE_ASSIGNMENT(Trait::TriviallyAvailable,
                                 constexpr MoveAssignment& operator=(MoveAssignment&& that)  = default);

        VARIANTX_MOVE_ASSIGNMENT(Trait::Available,
                                 constexpr MoveAssignment& operator=(MoveAssignment&& that) noexcept (((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_move_assignable_v<Ts>) && ...))
                                 {
                                    this->GenericAssign(std::move(that));
                                    return *this;
                                 } VARIANTX_EAT_SEMICOLON);

        VARIANTX_MOVE_ASSIGNMENT(Trait::Unavailable,
                                 constexpr MoveAssignment& operator=(MoveAssignment&& that)  = delete);

        #undef VARIANTX_MOVE_ASSIGNMENT
        // clang-format on

        // clang-format off
        template <typename Traits, Trait = Traits::kCopyAssignableTrait>
        class CopyAssignment;

        // NOLINTNEXTLINE
        #define VARIANTX_COPY_ASSIGNMENT(copy_assignable_trait, copy_assignment_definition)                 \
        template <typename... Ts>                                                                           \
        class CopyAssignment<Traits<Ts...>, copy_assignable_trait> : public MoveAssignment<Traits<Ts...>>   \
        {                                                                                                   \
            using BaseType = MoveAssignment<Traits<Ts...>>;                                                 \
                                                                                                            \
        public:                                                                                             \
            using BaseType::BaseType;                                                                       \
            using BaseType::operator=;                                                                      \
                                                                                                            \
            CopyAssignment(const CopyAssignment&) = default;                                                \
            CopyAssignment(CopyAssignment&&) = default;                                                     \
            ~CopyAssignment() = default;                                                                    \
            CopyAssignment& operator=(CopyAssignment&&)  = default;                                         \
                                                                                                            \
            copy_assignment_definition;                                                                     \
        }

        VARIANTX_COPY_ASSIGNMENT(Trait::TriviallyAvailable,
                                 constexpr CopyAssignment& operator=(const CopyAssignment& that) = default);

        VARIANTX_COPY_ASSIGNMENT(Trait::Available,
                                 constexpr CopyAssignment& operator=(const CopyAssignment& that)
                                 {
                                    this->GenericAssign(that);
                                    return *this;
                                 } VARIANTX_EAT_SEMICOLON);

        VARIANTX_COPY_ASSIGNMENT(Trait::Unavailable,
                                 constexpr CopyAssignment& operator=(const CopyAssignment& that) = delete);

        #undef VARIANTX_COPY_ASSIGNMENT
        // clang-format on

        template <typename... Ts>
        // NOLINTNEXTLINE
        class Impl : public CopyAssignment<Traits<Ts...>>
        {
            using BaseType = CopyAssignment<Traits<Ts...>>;

        public:
            using BaseType::BaseType;

            Impl(const Impl&)            = default;
            Impl(Impl&&)                 = default;
            Impl& operator=(const Impl&) = default;
            Impl& operator=(Impl&&)      = default;

            template <std::size_t Index, typename Arg>
            constexpr void Assign(Arg&& arg)
            {
                this->AssignAlternative(access::Base::GetAlternative<Index>(*this),
                                        std::forward<Arg>(arg));
            }

            // NOLINTNEXTLINE
            inline constexpr void Swap(Impl& that)
            {
                if (this->ValuelessByException() && that.ValuelessByException())
                {  // NOLINT
                   // do nothing
                }
                else if (this->Index() == that.Index())
                {
                    visitation::Base::VisitAlternativeAt(
                        this->Index(),
                        [](auto& this_alt, auto& that_alt)
                        {
                            using std::swap;
                            swap(this_alt.value_, that_alt.value_);
                        },
                        *this, that);
                }
                else
                {
                    Impl* lhs = this;
                    Impl* rhs = std::addressof(that);

                    if (lhs->MoveNothrow() || !rhs->MoveNothrow())
                    {
                        std::swap(lhs, rhs);
                    }

                    Impl tmp(std::move(*rhs));
                    if constexpr (std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
                    {
                        this->GenericConstruct(*rhs, std::move(*lhs));
                    }
                    else
                    {
                        try
                        {
                            this->GenericConstruct(*rhs, std::move(*lhs));
                        }
                        catch (...)
                        {
                            if (tmp.MoveNothrow())
                            {
                                this->GenericConstruct(*rhs, std::move(tmp));
                            }

                            throw;
                        }
                    }

                    this->GenericConstruct(*lhs, std::move(tmp));
                }
            }

        private:
            // NOLINTNEXTLINE
            constexpr inline bool MoveNothrow() const
            {
                // NOLINTNEXTLINE -> C-Style array
                constexpr bool kResults[] = {std::is_nothrow_move_constructible_v<Ts>...};
                return this->ValuelessByException() || kResults[this->Index()];
            }
        };
    }  // namespace impl

    template <std::size_t Index, typename T>
    struct VariantAlternative;

    template <std::size_t Index, typename T>
    using VariantAlternativeType = typename VariantAlternative<Index, T>::Type;

    template <typename Visitor, typename... Variants,
              typename = std::void_t<decltype(impl::AsVariant(std::declval<Variants>()))...>>
    constexpr decltype(auto) Visit(Visitor&& visitor, Variants&&... variants);

    template <typename Ret, typename Visitor, typename... Variants,
              typename = std::void_t<decltype(impl::AsVariant(std::declval<Variants>()))...>>
    constexpr Ret Visit(Visitor&& visitor, Variants&&... variants);

    template <typename... Ts>
    class Variant
        : private SfinaeCtorBase<std::conjunction_v<std::is_copy_constructible<Ts>...>,
                                 std::conjunction_v<std::is_move_constructible<Ts>...>>,
          private SfinaeAssignBase<
              ((std::is_copy_constructible_v<Ts> && std::is_copy_assignable_v<Ts>) && ...),
              ((std::is_move_constructible_v<Ts> && std::is_move_assignable_v<Ts>) && ...)>
    {
        static_assert(0 < sizeof...(Ts), "variant must consist of at least one alternative.");

        static_assert(!std::conjunction_v<std::is_array<Ts>...>,
                      "variant can not have an array type as an alternative.");

        static_assert(!std::conjunction_v<std::is_reference<Ts>...>,
                      "variant can not have a reference type as an alternative.");

        static_assert(!std::conjunction_v<std::is_void<Ts>...>,
                      "variant can not have a void type as an alternative.");

        using FirstType = VariantAlternativeType<0, Variant>;

    public:
        // clang-format off
        constexpr Variant()
            noexcept(std::is_nothrow_default_constructible_v<FirstType>)
            requires(std::is_default_constructible_v<FirstType>)
            : impl_(std::in_place_index_t<0>())
        {
        }
        // clang-format on

        constexpr Variant(const Variant&) = default;
        constexpr Variant(Variant&&)      = default;

        // clang-format off
        template <typename Arg,
                  typename T = utilities::SelectorType<Arg, Ts...>,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires (
                !std::is_same_v<std::remove_cvref_t<Arg>, Variant> &&
                !utilities::IsInplaceType<std::remove_cvref_t<Arg>>::value &&
                !utilities::IsInplaceIndex<std::remove_cvref_t<Arg>>::value &&
                std::is_constructible_v<T, Arg>
            )
        constexpr Variant(Arg&& arg) noexcept(std::is_nothrow_constructible_v<T, Arg>) // NOLINT -> non-explicit
            : impl_(std::in_place_index_t<Index>(), std::forward<Arg>(arg))
        {
        }
        // clang-format on

        // clang-format off
        template <std::size_t Index,
                  typename... Args,
                  typename = std::enable_if_t<(Index < sizeof...(Ts)), int>,
                  typename T = VariantAlternativeType<Index, Variant<Ts...>>>
            requires (
                std::is_constructible_v<T, Args...>
            )
        // NOLINTNEXTLINE -> unnamed parameter
        explicit constexpr Variant([[maybe_unused]] std::in_place_index_t<Index>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
            : impl_(std::in_place_index_t<Index>(), std::forward<Args>(args)...)
        {
        }
        // clang-format on

        // clang-format off
        template <std::size_t Index,
                  typename U,
                  typename... Args,
                  typename = std::enable_if_t<(Index < sizeof...(Ts)), int>,
                  typename T = VariantAlternativeType<Index, Variant<Ts...>>>
            requires(
                std::is_constructible_v<T, std::initializer_list<U>&, Args...>
            )
        // NOLINTNEXTLINE -> unnamed parameter
        explicit constexpr Variant([[maybe_unused]] std::in_place_index_t<Index>,
                                   std::initializer_list<U> list,
                                   Args&&... args) noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
            : impl_(std::in_place_index_t<Index>(), list, std::forward<Args>(args)...)
        {
        }
        // clang-format on

        // clang-format off
        template <typename T,
                  typename... Args,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires (
                std::is_constructible_v<T, Args...>
            )
        // NOLINTNEXTLINE -> unnamed parameter
        explicit constexpr Variant([[maybe_unused]] std::in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
            : impl_(std::in_place_index_t<Index>(), std::forward<Args>(args)...)
        {
        }
        // clang-format on

        // clang-format off
        template <typename T,
                  typename U,
                  typename... Args,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires (
                std::is_constructible_v<T, std::initializer_list<U>&, Args...>
            )
        // NOLINTNEXTLINE -> unnamed parameter
        explicit constexpr Variant([[maybe_unused]] std::in_place_type_t<T>,
                                   std::initializer_list<U> list,
                                   Args&&... args) noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
            : impl_(std::in_place_index_t<Index>(), list, std::forward<Args>(args)...)
        {
        }
        // clang-format on

        constexpr ~Variant() = default;

        constexpr Variant& operator=(const Variant&) = default;
        constexpr Variant& operator=(Variant&&)      = default;

        // clang-format off
        template <typename Arg,
                  typename T = utilities::SelectorType<Arg, Ts...>,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires (
                !std::is_same_v<Variant, std::remove_cvref_t<Arg>> &&
                std::is_assignable_v<T&, Arg> &&
                std::is_constructible_v<T, Arg>
            )
        constexpr Variant& operator=(Arg&& arg) noexcept(
            std::is_nothrow_assignable_v<T&, Arg> &&
            std::is_nothrow_constructible_v<T, Arg>
        )
        {
            impl_.template Assign<Index>(std::forward<Arg>(arg));
            return *this;
        }
        // clang-format on

        // clang-format off
        template <std::size_t Index,
                  typename... Args,
                  typename T = VariantAlternativeType<Index, Variant<Ts...>>>
            requires (
                Index < sizeof...(Ts) &&
                std::is_constructible_v<T, Args...>
            )
        constexpr T& Emplace(Args&&... args)
        {
            return impl_.template Emplace<Index>(std::forward<Args>(args)...);
        }
        // clang-format on

        // clang-format off
        template <std::size_t Index,
                  typename U,
                  typename... Args,
                  typename T = VariantAlternativeType<Index, Variant<Ts...>>>
            requires (
                Index < sizeof...(Ts) &&
                std::is_constructible_v<T, std::initializer_list<U>&, Args...>
            )
        constexpr T& Emplace(std::initializer_list<U> list, Args&&... args)
        {
            return impl_.template Emplace<Index>(list, std::forward<Args>(args)...);
        }
        // clang-format on

        // clang-format off
        template <typename T,
                  typename... Args,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires (
                std::is_constructible_v<T, Args...>
            )
        constexpr T& Emplace(Args&&... args)
        {
            return impl_.template Emplace<Index>(std::forward<Args>(args)...);
        }
        // clang-format on

        // clang-format off
        template <typename T,
                  typename U,
                  typename... Args,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires (
                std::is_constructible_v<T, std::initializer_list<U>&, Args...>
            )
        constexpr T& Emplace(std::initializer_list<U>& list, Args&&... args)
        {
            return impl_.template Emplace<Index>(list, std::forward<Args>(args)...);
        }
        // clang-format on

        constexpr bool ValuelessByException() const noexcept
        {
            return impl_.ValuelessByException();
        }

        constexpr std::size_t Index() const noexcept { return impl_.Index(); }

        // NOLINTNEXTLINE
        constexpr void swap(Variant& that) noexcept(
            ((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_swappable_v<Ts>) && ...))
            requires(((std::is_move_constructible_v<Ts> && std::is_swappable_v<Ts>) && ...))
        {
            impl_.Swap(that.impl_);
        }

    private:
        impl::Impl<Ts...> impl_;

        friend struct impl::access::Variant;
        friend struct impl::visitation::Variant;
    };

    namespace impl
    {
        template <std::size_t Index, typename... Ts>
        constexpr bool HoldsAlternative(const Variant<Ts...>& variant) noexcept
        {
            return Index == variant.Index();
        }
    }  // namespace impl

    template <typename T, typename... Ts>
    constexpr bool HoldsAlternative(const Variant<Ts...>& variant) noexcept
    {
        constexpr std::size_t kIndex = utilities::FindExactlyOne<T, Ts...>;
        return impl::HoldsAlternative<kIndex>(variant);
    }

    namespace impl
    {
        template <std::size_t Index, typename TVariant>
        constexpr auto&& GenericGet(TVariant&& variant)
        {
            using impl::access::Variant;
            if (!impl::HoldsAlternative<Index>(variant))
            {
                throw BadVariantAccess();
            }

            return Variant::GetAlternative<Index>(std::forward<TVariant>(variant)).value_;
        }
    }  // namespace impl

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>& Get(Variant<Ts...>& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return impl::GenericGet<Index>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>&& Get(Variant<Ts...>&& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return impl::GenericGet<Index>(std::move(variant));
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>& Get(
        const Variant<Ts...>& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return impl::GenericGet<Index>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>&& Get(
        const Variant<Ts...>&& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return impl::GenericGet<Index>(std::move(variant));
    }

    template <typename T, typename... Ts>
    constexpr T& Get(Variant<Ts...>& variant)
    {
        static_assert(!std::is_void_v<T>);
        return variantx::Get<utilities::FindExactlyOne<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr T&& Get(Variant<Ts...>&& variant)
    {
        static_assert(!std::is_void_v<T>);
        return variantx::Get<utilities::FindExactlyOne<T, Ts...>>(std::move(variant));
    }

    template <typename T, typename... Ts>
    constexpr const T& Get(const Variant<Ts...>& variant)
    {
        static_assert(!std::is_void_v<T>);
        return variantx::Get<utilities::FindExactlyOne<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr const T&& Get(const Variant<Ts...>&& variant)
    {
        static_assert(!std::is_void_v<T>);
        return variantx::Get<utilities::FindExactlyOne<T, Ts...>>(std::move(variant));
    }

    namespace impl
    {
        template <std::size_t Index, typename TVariant>
        constexpr auto* GenericGetIf(TVariant* variant) noexcept
        {
            using impl::access::Variant;

            // specifically conditional operator
            return variant != nullptr && HoldsAlternative<Index>(*variant)
                       ? std::addressof(Variant::GetAlternative<Index>(*variant).value_)
                       : nullptr;
        }
    }  // namespace impl

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        Variant<Ts...>* variant) noexcept
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return impl::GenericGetIf<Index>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        const Variant<Ts...>* variant) noexcept
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return impl::GenericGetIf<Index>(variant);
    }

    template <typename T, typename... Ts>
    constexpr std::add_pointer_t<T> GetIf(Variant<Ts...>* variant) noexcept
    {
        static_assert(!std::is_void_v<T>);
        return variantx::GetIf<utilities::FindExactlyOne<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr std::add_pointer_t<const T> GetIf(const Variant<Ts...>* variant) noexcept
    {
        static_assert(!std::is_void_v<T>);
        return variantx::GetIf<utilities::FindExactlyOne<T, Ts...>>(variant);
    }

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

    namespace impl
    {
        template <typename Operator>
        struct Convert2Bool
        {
            template <typename T, typename U>
            constexpr bool operator()(T&& fst, U&& snd) const
            {
                static_assert(
                    std::is_convertible_v<
                        decltype(Operator()(std::forward<T>(fst), std::forward<U>(fst))), bool>);

                return Operator()(std::forward<T>(fst), std::forward<U>(snd));
            }
        };
    }  // namespace impl

    template <typename... Ts>
    constexpr bool operator==(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        if (lhs.Index() != rhs.Index())
        {
            return false;
        }
        if (lhs.ValuelessByException())
        {
            return true;
        }

        return Variant::VisitValueAt(lhs.Index(), impl::Convert2Bool<std::equal_to<>>(), lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator!=(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        if (lhs.Index() != rhs.Index())
        {
            return true;
        }
        if (lhs.ValuelessByException())
        {
            return false;
        }

        return Variant::VisitValueAt(lhs.Index(), impl::Convert2Bool<std::not_equal_to<>>(), lhs,
                                     rhs);
    }

    template <typename... Ts>
    constexpr bool operator<(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        if (rhs.ValuelessByException())
        {
            return false;
        }
        if (lhs.ValuelessByException())
        {
            return true;
        }
        if (lhs.Index() < rhs.Index())
        {
            return true;
        }
        if (lhs.Index() > rhs.Index())
        {
            return false;
        }

        return Variant::VisitValueAt(lhs.Index(), impl::Convert2Bool<std::less<>>(), lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator>(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        if (lhs.ValuelessByException())
        {
            return false;
        }
        if (rhs.ValuelessByException())
        {
            return true;
        }
        if (lhs.Index() > rhs.Index())
        {
            return true;
        }
        if (lhs.Index() < rhs.Index())
        {
            return false;
        }

        return Variant::VisitValueAt(lhs.Index(), impl::Convert2Bool<std::greater<>>(), lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator<=(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        if (lhs.ValuelessByException())
        {
            return true;
        }
        if (rhs.ValuelessByException())
        {
            return false;
        }
        if (lhs.Index() < rhs.Index())
        {
            return true;
        }
        if (lhs.Index() > rhs.Index())
        {
            return false;
        }

        return Variant::VisitValueAt(lhs.Index(), impl::Convert2Bool<std::less_equal<>>(), lhs,
                                     rhs);
    }

    template <typename... Ts>
    constexpr bool operator>=(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        if (rhs.ValuelessByException())
        {
            return true;
        }
        if (lhs.ValuelessByException())
        {
            return false;
        }
        if (lhs.Index() > rhs.Index())
        {
            return true;
        }
        if (lhs.Index() < rhs.Index())
        {
            return false;
        }

        return Variant::VisitValueAt(lhs.Index(), impl::Convert2Bool<std::greater_equal<>>(), lhs,
                                     rhs);
    }

    template <typename... Ts>
        requires(std::three_way_comparable<Ts> && ...)
    constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...> operator<=>(
        const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using impl::visitation::Variant;
        using ResultType =
            std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...>;

        if (lhs.ValuelessByException() && rhs.ValuelessByException())
        {
            return std::strong_ordering::equal;
        }
        if (lhs.ValuelessByException())
        {
            return std::strong_ordering::less;
        }
        if (rhs.ValuelessByException())
        {
            return std::strong_ordering::greater;
        }
        if (auto result = lhs.Index() <=> rhs.Index(); result != 0)
        {
            return result;
        }

        auto three_way = []<typename T>(const T& fst, const T& snd) -> ResultType
        { return fst <=> snd; };

        return Variant::VisitValueAt(lhs.Index(), three_way, lhs, rhs);
    }

    namespace impl
    {
        template <typename... Variants>
        // NOLINTNEXTLINE
        constexpr void ThrowIfValueless(Variants&&... variants)
        {
            const bool valueless = (AsVariant(variants).ValuelessByException() || ...);
            if (valueless)
            {
                throw variantx::BadVariantAccess();
            }
        }
    }  // namespace impl

    template <typename Visitor, typename... Variants, typename>
    constexpr decltype(auto) Visit(Visitor&& visitor, Variants&&... variants)
    {
        using impl::visitation::Variant;

        impl::ThrowIfValueless(std::forward<Variants>(variants)...);
        return Variant::VisitValue(std::forward<Visitor>(visitor),
                                   std::forward<Variants>(variants)...);
    }

    template <typename Ret, typename Visitor, typename... Variants, typename>
    constexpr Ret Visit(Visitor&& visitor, Variants&&... variants)
    {
        using impl::visitation::Variant;

        impl::ThrowIfValueless(std::forward<Variants>(variants)...);
        return Variant::VisitValue<Ret>(std::forward<Visitor>(visitor),
                                        std::forward<Variants>(variants)...);
    }

    template <typename... Ts>
    // NOLINTNEXTLINE
    constexpr auto swap(Variant<Ts...>& lhs, Variant<Ts...>& rhs) noexcept(noexcept(lhs.swap(rhs)))
        -> decltype(lhs.swap(rhs))
    {
        return lhs.swap(rhs);
    }
}  // namespace variantx
