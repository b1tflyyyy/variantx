#pragma once

#include <cstdint>
#include <functional>
#include <type_traits>

#include "fwd/variantx.hpp"
#include "utilities.hpp"
#include "variantx-exceptions.hpp"

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

    namespace detail
    {
        struct TraitImpl
        {
            using TriviallyAvailable = std::integral_constant<std::uint32_t, 0>;
            using Available          = std::integral_constant<std::uint32_t, 1>;
            using Unavailable        = std::integral_constant<std::uint32_t, 2>;
        };

        // clang-format off
        template <typename T,
                  template <typename> typename IsTriviallyAvailable,
                  template <typename> typename IsAvailable>
        using Trait = std::conditional_t<IsTriviallyAvailable<T>::value, TraitImpl::TriviallyAvailable,
                      std::conditional_t<IsAvailable<T>::value, TraitImpl::Available, TraitImpl::Unavailable>>;
        // clang-format on

        class CommonTraitImpl
        {
            template <typename T, typename... Ts>
            static constexpr bool kContains = (std::is_same_v<T, Ts> || ...);

        public:
            // clang-format off
            template <typename... Ts>
            using Type = std::conditional_t<kContains<TraitImpl::Unavailable, Ts...>, TraitImpl::Unavailable,
                         std::conditional_t<kContains<TraitImpl::Available, Ts...>, TraitImpl::Available, TraitImpl::TriviallyAvailable>>;
            // clang-format on
        };

        template <typename... Ts>
        using CommonTrait = CommonTraitImpl::Type<Ts...>;

        template <typename... Ts>
        struct Traits
        {
            // clang-format off
            using CopyConstructibleTrait =
                CommonTrait<Trait<Ts, std::is_trivially_copy_constructible, std::is_copy_constructible>...>;

            using MoveConstructibleTrait =
                CommonTrait<Trait<Ts, std::is_trivially_move_constructible, std::is_move_constructible>...>;

            using CopyAssignableTrait =
                CommonTrait<CopyConstructibleTrait, Trait<Ts, std::is_trivially_copy_assignable, std::is_copy_assignable>...>;

            using MoveAssignableTrait =
                CommonTrait<MoveConstructibleTrait, Trait<Ts, std::is_trivially_move_assignable, std::is_move_assignable>...>;

            using DestructibleTrait =
                CommonTrait<Trait<Ts, std::is_trivially_destructible, std::is_destructible>...>;
            // clang-format on
        };

        template <typename Tr>
        concept TriviallyAvailable = std::same_as<Tr, TraitImpl::TriviallyAvailable>;

        template <typename Tr>
        concept Available = std::same_as<Tr, TraitImpl::Available>;

        template <typename Tr>
        concept Unavailable = std::same_as<Tr, TraitImpl::Unavailable>;

        namespace access
        {
            struct VUnion
            {
                template <typename VUni>
                static constexpr auto&& Get(std::in_place_index_t<0>, VUni&& vuni)
                {
                    return std::forward<VUni>(vuni).alt_;
                }

                template <std::size_t Index, typename VUni>
                static constexpr auto&& Get(std::in_place_index_t<Index>, VUni&& vuni)
                {
                    return Get(std::in_place_index_t<Index - 1>{}, std::forward<VUni>(vuni).rest_);
                }
            };

            struct Impl
            {
                template <std::size_t Index, typename Imp>
                static constexpr auto&& Get(Imp&& imp)
                {
                    return VUnion::Get(std::in_place_index_t<Index>{}, std::forward<Imp>(imp).vunion_);
                }
            };

            struct Variant
            {
                template <std::size_t Index, typename Var>
                static constexpr auto&& Get(Var&& var)
                {
                    return Impl::Get<Index>(std::forward<Var>(var).impl_);
                }
            };
        }  // namespace access

        namespace visitation
        {
            class Impl
            {
            public:
                /*
                 * Visit an Alternative wrapper.
                 * Note: used for Copy/Move/etc... ctors/ops.
                 */
                template <typename Func, typename... Variants>
                static constexpr decltype(auto) VisitAlternativeAt(std::size_t index, Func&& func, Variants&&... variants)
                {
                    static constexpr auto kDiagonalFunctionArray = MakeDiagonalDispatchImpl<decltype(func), decltype(variants)...>();

                    const auto& f = kDiagonalFunctionArray[index];
                    return f(std::forward<Func>(func), std::forward<Variants>(variants)...);
                }

                /*
                 * Visit an Alternative wrapper.
                 */
                template <typename Func, typename... Variants>
                static constexpr decltype(auto) VisitAlternative(Func&& func, Variants&&... variants)
                {
                    static constexpr auto kMatrix = MakeFunctionalMatrixImpl<decltype(func), decltype(variants)...>();
                    const auto&           f       = AtN(kMatrix, std::forward<Variants>(variants).Index()...);

                    return f(std::forward<Func>(func), std::forward<Variants>(variants)...);
                }

            private:
                template <typename T>
                static constexpr const T& AtN(const T& element)
                {
                    return element;  // NOLINT use-after-free if prval
                }

                template <typename T, std::size_t N, typename... Indices>
                static constexpr auto&& AtN(const std::array<T, N>& nfunction_array, std::size_t index, Indices... indices)
                {
                    return AtN(nfunction_array[index], indices...);
                }

                template <typename Func, typename... Funcs>
                static constexpr void CheckFunctionsIdenticalType() noexcept
                {
                    static_assert((std::is_same_v<std::remove_cvref_t<Func>, std::remove_cvref_t<Funcs>> && ...));
                }

                template <typename... Funcs>
                static constexpr auto MakeFunctionalArray(Funcs&&... funcs)
                {
                    CheckFunctionsIdenticalType<Funcs...>();

                    using CommonFuncType = std::common_type_t<std::remove_cvref_t<Funcs>...>;
                    return std::array<CommonFuncType, sizeof...(Funcs)>{std::forward<Funcs>(funcs)...};
                }

                template <std::size_t... Is>
                struct Dispatcher
                {
                    template <typename Func, typename... Variants>
                    static constexpr decltype(auto) Dispatch(Func func, Variants... variants)
                    {
                        static_assert(sizeof...(Is) == sizeof...(Variants));
                        return std::invoke(static_cast<Func>(func), access::Impl::Get<Is>(static_cast<Variants>(variants))...);
                    };
                };

                // clang-format off
                template <typename Func, std::size_t Index, typename... Variants>
                static constexpr decltype(auto) MakeDiagonalDispatchImpl()
                {
                    using Indices = std::index_sequence<((void)sizeof(Variants), Index)...>;
                    return [&]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
                        return &Dispatcher<Is...>::template Dispatch<Func, Variants...>;
                    }(Indices{});
                }

                template <typename Func, typename Variant, typename... Variants>
                static constexpr decltype(auto) MakeDiagonalDispatchImpl()
                {
                    static constexpr std::size_t kSz = std::remove_cvref_t<Variant>::Size();
                    static_assert(((kSz == std::remove_cvref_t<Variants>::Size()) && ...));

                    using Indices = std::make_index_sequence<kSz>;
                    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                         return MakeFunctionalArray(MakeDiagonalDispatchImpl<Func, Is, Variant, Variants...>()...);
                    }(Indices{});
                }

                template <typename Func, typename... Variants, std::size_t... Indices>
                static constexpr decltype(auto) MakeFunctionalMatrixImpl(std::index_sequence<Indices...>)
                {
                    return &Dispatcher<Indices...>::template Dispatch<Func, Variants...>;
                }

                template <typename Func, typename... Variants, std::size_t... CurrentIs, std::size_t... NextIs, typename... RestLayers>
                static constexpr decltype(auto) MakeFunctionalMatrixImpl(std::index_sequence<CurrentIs...>,
                                                                         std::index_sequence<NextIs...>,
                                                                         RestLayers... rest)
                {
                    return MakeFunctionalArray(MakeFunctionalMatrixImpl<Func, Variants...>(std::index_sequence<CurrentIs..., NextIs>{}, rest...)...);
                }

                template <typename Func, typename... Variants>
                static constexpr decltype(auto) MakeFunctionalMatrixImpl()
                {
                    return MakeFunctionalMatrixImpl<Func, Variants...>(std::index_sequence<>{},
                                                                       std::make_index_sequence<std::remove_cvref_t<Variants>::Size()>{}...);
                }
                // clang-format on
            };

            class Variant
            {
            public:
                template <typename Func, typename... Vars>
                static constexpr decltype(auto) VisitAlternative(Func&& func, Vars&&... vars)
                {
                    return Impl::VisitAlternative(MakeValueVisitor(std::forward<Func>(func)), std::forward<Vars>(vars).impl_...);
                }

                template <typename Ret, typename Func, typename... Vars>
                static constexpr Ret VisitAlternative(Func&& func, Vars&&... vars)
                {
                    return Impl::VisitAlternative(MakeRetValueVisitor<Ret>(std::forward<Func>(func)), std::forward<Vars>(vars).impl_...);
                }

                template <typename Func, typename... Vars>
                static constexpr decltype(auto) VisitAlternativeAt(std::size_t index, Func&& func, Vars&&... vars)
                {
                    return Impl::VisitAlternativeAt(index, MakeValueVisitor(std::forward<Func>(func)), std::forward<Vars>(vars).impl_...);
                }

            private:
                template <typename Func, typename... Vals>
                static constexpr void InvocableWithVals()
                {
                    static_assert(std::is_invocable_v<Func, Vals...>);
                }

                template <typename Func>
                struct ValueVisitor
                {
                    template <typename... Alts>
                    constexpr decltype(auto) operator()(Alts&&... alts)
                    {
                        InvocableWithVals<Func, decltype((std::forward<Alts>(alts).value_))...>();
                        return std::invoke(std::forward<Func>(func_), std::forward<Alts>(alts).value_...);
                    }

                    Func&& func_;
                };

                template <typename Ret, typename Func>
                struct RetValueVisitor
                {
                    template <typename... Alts>
                    constexpr Ret operator()(Alts&&... alts)
                    {
                        InvocableWithVals<Func, decltype((std::forward<Alts>(alts).value_))...>();

                        if constexpr (std::is_same_v<Ret, void>)
                        {
                            std::invoke(std::forward<Func>(func_), std::forward<Alts>(alts).value_...);
                        }
                        else
                        {
                            return std::invoke(std::forward<Func>(func_), std::forward<Alts>(alts).value_...);
                        }
                    }

                    Func&& func_;
                };

                template <typename Func>
                static constexpr auto MakeValueVisitor(Func&& func)
                {
                    return ValueVisitor<Func>{std::forward<Func>(func)};
                }

                template <typename Ret, typename Func>
                static constexpr auto MakeRetValueVisitor(Func&& func)
                {
                    return RetValueVisitor<Ret, Func>{std::forward<Func>(func)};
                }
            };
        }  // namespace visitation

        template <typename T, std::size_t Index>
        struct Alternative
        {
            using ValueType                     = T;
            static constexpr std::size_t kIndex = Index;

            template <typename... Args>
            constexpr explicit Alternative(std::in_place_t, Args&&... args) : value_(std::forward<Args>(args)...)
            {
            }

            T value_;
        };

        struct ValuelessTag
        {
        };

        template <typename, std::size_t, typename...>
        union VUnion;

        template <typename DestructibleTrait, std::size_t Index>
        union VUnion<DestructibleTrait, Index>
        {
        };

        template <typename DestructibleTrait, std::size_t Index, typename Head, typename... Rest>
        union VUnion<DestructibleTrait, Index, Head, Rest...>
        {
            friend struct access::VUnion;

        public:
            constexpr explicit VUnion(ValuelessTag) noexcept : dummy_{} {}

            template <typename... Args>
            constexpr explicit VUnion(std::in_place_index_t<0>, Args&&... args) : alt_(std::in_place_t{}, std::forward<Args>(args)...)
            {
            }

            template <std::size_t PIndex, typename... Args>
            constexpr explicit VUnion(std::in_place_index_t<PIndex>, Args&&... args) : rest_(std::in_place_index_t<PIndex - 1>{}, std::forward<Args>(args)...)
            {
            }

            constexpr VUnion(const VUnion&)            = default;
            constexpr VUnion& operator=(const VUnion&) = default;

            constexpr VUnion(VUnion&&)            = default;
            constexpr VUnion& operator=(VUnion&&) = default;

            constexpr ~VUnion()
                requires TriviallyAvailable<DestructibleTrait>
            = default;

            constexpr ~VUnion()
                requires Available<DestructibleTrait>
            {
            }

            constexpr ~VUnion()
                requires Unavailable<DestructibleTrait>
            = delete;

        private:
            char                     dummy_;  // valueless_by_exception
            Alternative<Head, Index> alt_;

            VUnion<DestructibleTrait, Index + 1, Rest...> rest_;
        };

        template <typename... Ts>
        class VariantImpl
        {
            friend struct access::Impl;
            friend class visitation::Impl;

            using CopyConstructibleTrait = Traits<Ts...>::CopyConstructibleTrait;
            using MoveConstructibleTrait = Traits<Ts...>::MoveConstructibleTrait;
            using CopyAssignableTrait    = Traits<Ts...>::CopyAssignableTrait;
            using MoveAssignableTrait    = Traits<Ts...>::MoveAssignableTrait;
            using DestructibleTrait      = Traits<Ts...>::DestructibleTrait;

        public:
            constexpr explicit VariantImpl(ValuelessTag tag) : index_(kVariantNpos), vunion_(tag) {}

            template <std::size_t Index, typename... Args>
            constexpr explicit VariantImpl(std::in_place_index_t<Index>, Args&&... args)
                : index_(Index), vunion_(std::in_place_index_t<Index>(), std::forward<Args>(args)...)
            {
            }

            constexpr std::size_t Index() const noexcept { return index_; }
            constexpr bool        ValuelessByException() const noexcept { return Index() == kVariantNpos; }

            constexpr ~VariantImpl()
                requires TriviallyAvailable<DestructibleTrait>
            = default;

            constexpr ~VariantImpl()
                requires Available<DestructibleTrait>
            {
                Destroy();
            }

            constexpr ~VariantImpl()
                requires Unavailable<DestructibleTrait>
            = delete;

            constexpr VariantImpl(const VariantImpl&)
                requires TriviallyAvailable<CopyConstructibleTrait>
            = default;

            constexpr VariantImpl(const VariantImpl& that)
                requires Available<CopyConstructibleTrait>
                : VariantImpl(ValuelessTag{})
            {
                GenericConstructFrom(*this, that);
            }

            constexpr VariantImpl(const VariantImpl&)
                requires Unavailable<CopyConstructibleTrait>
            = delete;

            constexpr VariantImpl(VariantImpl&& that)
                requires TriviallyAvailable<MoveConstructibleTrait>
            = default;

            constexpr VariantImpl(VariantImpl&& that) noexcept((std::is_nothrow_move_constructible_v<Ts> && ...))
                requires Available<MoveConstructibleTrait>
                : VariantImpl(ValuelessTag{})
            {
                GenericConstructFrom(*this, std::move(that));
            }

            constexpr VariantImpl(VariantImpl&& that)
                requires Unavailable<MoveConstructibleTrait>
            = delete;

            constexpr VariantImpl& operator=(const VariantImpl&)
                requires TriviallyAvailable<CopyAssignableTrait>
            = default;

            constexpr VariantImpl& operator=(const VariantImpl& that)
                requires Available<CopyAssignableTrait>
            {
                GenericAssignFrom(*this, that);
                return *this;
            }

            constexpr VariantImpl& operator=(const VariantImpl&)
                requires Unavailable<CopyAssignableTrait>
            = delete;

            constexpr VariantImpl& operator=(VariantImpl&&)
                requires TriviallyAvailable<MoveAssignableTrait>
            = default;

            constexpr VariantImpl& operator=(VariantImpl&& that) noexcept(
                ((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_move_assignable_v<Ts>) && ...))
                requires Available<MoveAssignableTrait>
            {
                GenericAssignFrom(*this, std::move(that));
                return *this;
            }

            constexpr VariantImpl& operator=(VariantImpl&&)
                requires Unavailable<MoveAssignableTrait>
            = delete;

            template <std::size_t Index, typename... Args>
            constexpr auto& Emplace(Args&&... args)
            {
                Destroy();
                std::construct_at(std::addressof(vunion_), std::in_place_index_t<Index>{}, std::forward<Args>(args)...);

                index_ = Index;
                return access::Impl::Get<Index>(*this).value_;
            }

            template <std::size_t Index, typename Arg>
            constexpr void Assign(Arg&& arg)
            {
                AssignAltFromOther(*this, Alternative<Arg&&, Index>{std::in_place_t{}, std::forward<Arg>(arg)});
            }

            constexpr void Swap(VariantImpl& that)
            {
                if (ValuelessByException() && that.ValuelessByException())
                {
                    // do nothing.
                }
                else if (Index() == that.Index())
                {
                    visitation::Impl::VisitAlternativeAt(
                        Index(),
                        [](auto& lalt, auto& ralt)
                        {
                            using std::swap;
                            swap(lalt.value_, ralt.value_);
                        },
                        *this, that);
                }
                else
                {
                    VariantImpl tmp = std::move(that);
                    GenericConstructFrom(that, std::move(*this));
                    GenericConstructFrom(*this, std::move(tmp));
                }
            }

        private:
            constexpr void Destroy() noexcept
                requires TriviallyAvailable<DestructibleTrait>
            {
                index_ = kVariantNpos;
            };

            constexpr void Destroy() noexcept
                requires Available<DestructibleTrait>
            {
                if (!ValuelessByException())
                {
                    visitation::Impl::VisitAlternativeAt(
                        index_,
                        [](auto&& value)
                        {
                            using T = std::remove_cvref_t<decltype(value)>;
                            value.~T();
                        },
                        *this);

                    index_ = kVariantNpos;
                }
            }

            constexpr void Destroy() noexcept
                requires Unavailable<DestructibleTrait>
            = delete;

            static constexpr std::size_t Size() noexcept { return sizeof...(Ts); }

            template <typename From>
            static constexpr void GenericConstructFrom(VariantImpl& me, From&& from)
            {
                me.Destroy();

                if (!from.ValuelessByException())
                {
                    // clang-format off
                    visitation::Impl::VisitAlternativeAt(from.Index(),
                        [&](auto /* Alternative<...> */ && value)
                        {
                            std::construct_at(std::addressof(me.vunion_),
                                              std::in_place_index_t<std::decay_t<decltype(value)>::kIndex>{},
                                              std::forward<decltype(value)>(value).value_);
                        }, std::forward<From>(from));
                    // clang-format on

                    me.index_ = from.Index();
                }
            }

            template <typename OtherAlt>
            static constexpr void AssignAltFromOther(VariantImpl& me, OtherAlt&& other_alt)
            {
                static constexpr std::size_t kOtherIndex = std::remove_cvref_t<OtherAlt>::kIndex;

                if (me.Index() == kOtherIndex)
                {
                    access::Impl::Get<kOtherIndex>(me).value_ = std::forward<OtherAlt>(other_alt).value_;
                    return;
                }

                using ArgType    = decltype((std::forward<OtherAlt>(other_alt).value_));
                using TargetType = VariantAlternativeType<kOtherIndex, Variant<Ts...>>;

                /*
                 * Strong guarantee.
                 */
                if constexpr (std::is_nothrow_constructible_v<TargetType, ArgType> || !std::is_nothrow_move_constructible_v<TargetType>)
                {
                    me.Emplace<kOtherIndex>(std::forward<OtherAlt>(other_alt).value_);
                }
                else
                {
                    me.Emplace<kOtherIndex>(TargetType(std::forward<OtherAlt>(other_alt).value_));
                }
            }

            template <typename From>
            static constexpr void GenericAssignFrom(VariantImpl& me, From&& from)
            {
                if (me.ValuelessByException() && from.ValuelessByException())
                {
                    // do nothing
                }
                else if (from.ValuelessByException())
                {
                    me.Destroy();
                }
                else
                {
                    // clang-format off
                    visitation::Impl::VisitAlternativeAt(
                        from.Index(),
                        [&]<typename FromAlt>(FromAlt&& from_alt) {
                            AssignAltFromOther(me, std::forward<FromAlt>(from_alt));
                        },
                        std::forward<From>(from));
                    // clang-format on
                }
            }

        private:
            std::size_t                         index_;
            VUnion<DestructibleTrait, 0, Ts...> vunion_;
        };
    }  // namespace detail

    template <typename... Ts>
    class Variant
    {
        static_assert(0 < sizeof...(Ts), "variant must consist of at least one alternative.");
        static_assert(!(std::is_array_v<Ts> || ...), "variant can not have an array type as an alternative.");
        static_assert(!(std::is_reference_v<Ts> || ...), "variant can not have a reference type as an alternative.");
        static_assert(!(std::is_void_v<Ts> || ...), "variant can not have a void type as an alternative.");

        using FirstType = VariantAlternativeType<0, Variant>;

    public:
        constexpr Variant() noexcept(std::is_nothrow_constructible_v<FirstType>)
            requires std::is_default_constructible_v<FirstType>
            : impl_(std::in_place_index_t<0>{})
        {
        }

        // clang-format off
        template <typename Arg,
                  typename T = utilities::SelectorType<Arg, Ts...>,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
        requires(!std::is_same_v<std::remove_cvref_t<Arg>, Variant> &&
                 !utilities::IsInplaceType<std::remove_cvref_t<Arg>>::value &&
                 !utilities::IsInplaceIndex<std::remove_cvref_t<Arg>>::value &&
                 std::is_constructible_v<T, Arg>)
        constexpr Variant(Arg&& arg) noexcept(std::is_nothrow_constructible_v<T, Arg>)
            : impl_(std::in_place_index_t<Index>{}, std::forward<Arg>(arg))
        {
        }

        template <std::size_t Index,
                  typename... Args>
        requires (Index < sizeof...(Ts) && std::constructible_from<VariantAlternativeType<Index, Variant<Ts...>>, Args...>)
        constexpr explicit Variant(std::in_place_index_t<Index>, Args&&... args)
            noexcept (std::is_nothrow_constructible_v<VariantAlternativeType<Index, Variant<Ts...>>, Args...>)
            : impl_(std::in_place_index_t<Index>{}, std::forward<Args>(args)...)
            {
            }

        template <std::size_t Index,
                  typename U,
                  typename... Args>
        requires (Index < sizeof...(Ts) && std::constructible_from<VariantAlternativeType<Index, Variant<Ts...>>, std::initializer_list<U>&, Args...>)
            explicit constexpr Variant(std::in_place_index_t<Index>,
                                       std::initializer_list<U> list,
                                       Args&&... args)
            noexcept(std::is_nothrow_constructible_v<VariantAlternativeType<Index, Variant<Ts...>>, std::initializer_list<U>&, Args...>)
                : impl_(std::in_place_index_t<Index>{}, list, std::forward<Args>(args)...)
            {
            }

        template <typename T,
                  typename... Args,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
        requires (std::is_constructible_v<T, Args...>)
            explicit constexpr Variant(std::in_place_type_t<T>, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
                : impl_(std::in_place_index_t<Index>(), std::forward<Args>(args)...)
            {
            }

        template <typename T,
                  typename U,
                  typename... Args,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
        requires (std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
            explicit constexpr Variant(std::in_place_type_t<T>,
                                       std::initializer_list<U> list,
                                       Args&&... args) noexcept(std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>)
                : impl_(std::in_place_index_t<Index>(), list, std::forward<Args>(args)...)
            {
            }

        template <typename Arg,
                  typename T = utilities::SelectorType<Arg, Ts...>,
                  std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
        requires (!std::same_as<Variant, std::remove_cvref_t<Arg>> && std::assignable_from<T&, Arg> && std::constructible_from<T, Arg>)
            constexpr Variant& operator=(Arg&& arg) noexcept(std::is_nothrow_assignable_v<T&, Arg> && std::is_nothrow_constructible_v<T, Arg>)
            {
                impl_.template Assign<Index>(std::forward<Arg>(arg));
                return *this;
            }
        // clang-format on

        template <std::size_t Index, typename... Args, typename T = VariantAlternativeType<Index, Variant<Ts...>>>
            requires(Index < sizeof...(Ts) && std::constructible_from<T, Args...>)
        constexpr T& Emplace(Args&&... args)
        {
            return impl_.template Emplace<Index>(std::forward<Args>(args)...);
        }

        template <std::size_t Index, typename U, typename... Args, typename T = VariantAlternativeType<Index, Variant<Ts...>>>
            requires(Index < sizeof...(Ts) && std::constructible_from<T, std::initializer_list<U>&, Args...>)
        constexpr T& Emplace(std::initializer_list<U> list, Args&&... args)
        {
            return impl_.template Emplace<Index>(list, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args, std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires(std::constructible_from<T, Args...>)
        constexpr T& Emplace(Args&&... args)
        {
            return impl_.template Emplace<Index>(std::forward<Args>(args)...);
        }

        template <typename T, typename U, typename... Args, std::size_t Index = utilities::FindUnambiguousIndex<T, Ts...>::value>
            requires(std::constructible_from<T, std::initializer_list<U>&, Args...>)
        constexpr T& Emplace(std::initializer_list<U>& list, Args&&... args)
        {
            return impl_.template Emplace<Index>(list, std::forward<Args>(args)...);
        }

        constexpr bool        ValuelessByException() const noexcept { return impl_.ValuelessByException(); }
        constexpr std::size_t Index() const noexcept { return impl_.Index(); }

        // NOLINTNEXTLINE -> code-style
        void swap(Variant& that) noexcept(((std::is_nothrow_move_constructible_v<Ts> && std::is_nothrow_swappable_v<Ts>) && ...))
            requires(((std::is_move_constructible_v<Ts> && std::is_swappable_v<Ts>) && ...))
        {
            impl_.Swap(that.impl_);
        }

    private:
        detail::VariantImpl<Ts...> impl_;

        friend struct detail::access::Variant;
        friend class detail::visitation::Variant;
    };

    namespace detail
    {
        template <std::size_t Index, typename... Ts>
        constexpr bool HoldsAlternativeImpl(const Variant<Ts...>& variant) noexcept
        {
            return Index == variant.Index();
        }

        template <std::size_t Index, typename Var>
        constexpr auto&& GenericGet(Var&& var)
        {
            if (!HoldsAlternativeImpl<Index>(var))
            {
                throw BadVariantAccess();
            }

            return detail::access::Variant::Get<Index>(std::forward<Var>(var)).value_;
        }
    }  // namespace detail

    template <typename T, typename... Ts>
    constexpr bool HoldsAlternative(const Variant<Ts...>& variant) noexcept
    {
        constexpr std::size_t kIndex = utilities::FindExactlyOne<T, Ts...>;
        return detail::HoldsAlternativeImpl<kIndex>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>& Get(Variant<Ts...>& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return detail::GenericGet<Index>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>&& Get(Variant<Ts...>&& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return detail::GenericGet<Index>(std::move(variant));
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>& Get(const Variant<Ts...>& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return detail::GenericGet<Index>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>&& Get(const Variant<Ts...>&& variant)
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return detail::GenericGet<Index>(std::move(variant));
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

    namespace detail
    {
        template <std::size_t Index, typename Var>
        constexpr auto* GenericGetIf(Var* var) noexcept
        {
            return var != nullptr && HoldsAlternativeImpl<Index>(*var) ? std::addressof(detail::access::Variant::Get<Index>(*var).value_) : nullptr;
        }
    }  // namespace detail

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Ts...>>> GetIf(Variant<Ts...>* variant) noexcept
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return detail::GenericGetIf<Index>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Ts...>>> GetIf(const Variant<Ts...>* variant) noexcept
    {
        static_assert(Index < sizeof...(Ts));
        static_assert(!std::is_void_v<VariantAlternativeType<Index, Variant<Ts...>>>);

        return detail::GenericGetIf<Index>(variant);
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

    namespace detail
    {
        template <typename Operator>
        struct OpWrapper
        {
            template <typename T, typename U>
            constexpr bool operator()(T&& fst, U&& snd) const
            {
                static_assert(std::is_convertible_v<decltype(Operator{}(std::forward<T>(fst), std::forward<U>(snd))), bool>);
                return Operator{}(std::forward<T>(fst), std::forward<U>(snd));
            }
        };
    }  // namespace detail

    template <typename... Ts>
    constexpr bool operator==(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        if (lhs.Index() != rhs.Index())
        {
            return false;
        }
        if (lhs.ValuelessByException())
        {
            return true;
        }

        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), detail::OpWrapper<std::equal_to<>>{}, lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator!=(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        if (lhs.Index() != rhs.Index())
        {
            return true;
        }
        if (lhs.ValuelessByException())
        {
            return false;
        }

        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), detail::OpWrapper<std::not_equal_to<>>{}, lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator<(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
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

        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), detail::OpWrapper<std::less<>>{}, lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator>(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
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

        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), detail::OpWrapper<std::greater<>>{}, lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator<=(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
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

        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), detail::OpWrapper<std::less_equal<>>{}, lhs, rhs);
    }

    template <typename... Ts>
    constexpr bool operator>=(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
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

        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), detail::OpWrapper<std::greater_equal<>>{}, lhs, rhs);
    }

    template <typename... Ts>
        requires(std::three_way_comparable<Ts> && ...)
    constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...> operator<=>(const Variant<Ts...>& lhs, const Variant<Ts...>& rhs)
    {
        using ResultType = std::common_comparison_category_t<std::compare_three_way_result_t<Ts>...>;

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

        auto three_way = [](const auto& fst, const auto& snd) -> ResultType { return fst <=> snd; };
        return detail::visitation::Variant::VisitAlternativeAt(lhs.Index(), three_way, lhs, rhs);
    }

    namespace detail
    {
        template <typename... Variants>
        constexpr void ThrowIfValuelessByException(const Variants&... variants)
        {
            if ((variants.ValuelessByException() || ...))
            {
                throw BadVariantAccess{};
            }
        }
    }  // namespace detail

    template <typename Visitor, typename... Variants>
    constexpr decltype(auto) Visit(Visitor&& visitor, Variants&&... variants)
    {
        detail::ThrowIfValuelessByException(variants...);
        return detail::visitation::Variant::VisitAlternative(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
    }

    template <typename Ret, typename Visitor, typename... Variants>
    constexpr Ret Visit(Visitor&& visitor, Variants&&... variants)
    {
        detail::ThrowIfValuelessByException(variants...);
        return detail::visitation::Variant::VisitAlternative<Ret>(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
    }

    template <typename... Ts>
    constexpr auto swap(Variant<Ts...>& lhs, Variant<Ts...>& rhs) noexcept(noexcept(lhs.swap(rhs))) -> decltype(lhs.swap(rhs))
    {
        lhs.swap(rhs);
    }
}  // namespace variantx
