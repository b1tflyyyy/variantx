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
        /* Friends */
        template <std::size_t Index, typename... Args>
        friend constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Args...>>> GetIf(
            Variant<Args...>* variant) noexcept;

        template <std::size_t Index, typename... Args>
        friend constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Args...>>>
        GetIf(const Variant<Args...>* variant) noexcept;

        template <typename Target, typename... Args>
        friend constexpr std::add_pointer_t<Target> GetIf(Variant<Args...>* variant) noexcept;

        template <typename Target, typename... Args>
        friend constexpr std::add_pointer_t<const Target> GetIf(
            const Variant<Args...>* variant) noexcept;

    public:
        constexpr Variant() noexcept : index_(0)
        {
            using Type = utilities::GetTypeByIndex<0, Ts...>;
            static_assert(std::is_nothrow_constructible_v<Type>,
                          "Type at index 0 is not nothrow default constructible!");

            new (std::data(storage_)) Type();
        }

        template <typename T>
            requires(!std::is_same_v<std::remove_cvref_t<T>, Variant<Ts...>> &&
                     utilities::SelectorValue<std::remove_cvref_t<T>, Ts...>)
        constexpr explicit Variant(T&& object)
            : index_(utilities::GetTypeIndex<std::remove_cvref_t<T>, Ts...>)
        {
            using Pure = std::remove_cvref_t<T>;
            static_assert((std::is_same_v<Pure, Ts> || ...), "Variant does not contain type T");

            new (std::data(storage_)) Pure(std::forward<T>(object));
        }

        constexpr Variant(const Variant<Ts...>& rhs)
            requires(!std::conjunction_v<std::is_copy_constructible<Ts>...>)
        = delete;

        constexpr Variant(const Variant<Ts...>& rhs)
            requires(std::conjunction_v<std::is_copy_constructible<Ts>...>)
            : index_(rhs.index_)
        {
            if (!rhs.ValuelessByException())
            {
                CopyConstruct<0>(rhs);
            }
        }

        constexpr Variant(Variant<Ts...>&& rhs) noexcept
            requires(!std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
        = delete;

        constexpr Variant(Variant<Ts...>&& rhs) noexcept
            requires(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...>)
            : index_(rhs.index_)
        {
            if (!rhs.ValuelessByException())
            {
                MoveConstruct<0>(std::move(rhs));
            }
        }

        constexpr Variant& operator=(const Variant<Ts...>& rhs)
            requires(!std::conjunction_v<std::is_copy_constructible<Ts>...> ||
                     !std::conjunction_v<std::is_copy_assignable<Ts>...>)
        = delete;

        constexpr Variant& operator=(const Variant<Ts...>& rhs)
            requires(std::conjunction_v<std::is_copy_constructible<Ts>...> &&
                     std::conjunction_v<std::is_copy_assignable<Ts>...>)
        {
            if ((this == &rhs) || (ValuelessByException() && rhs.ValuelessByException()))
            {
                return *this;
            }

            if (!ValuelessByException() && rhs.ValuelessByException())
            {
                Destroy<0>();
                index_ = variant_npos;
            }
            else
            {
                CopyAssign<0>(rhs);
            }

            return *this;
        }

        constexpr Variant& operator=(Variant<Ts...>&& rhs) noexcept
            requires(!std::conjunction_v<std::is_nothrow_move_constructible<Ts>...> ||
                     !std::conjunction_v<std::is_nothrow_move_assignable<Ts>...>)
        = delete;

        constexpr Variant& operator=(Variant<Ts...>&& rhs) noexcept
            requires(std::conjunction_v<std::is_nothrow_move_constructible<Ts>...> &&
                     std::conjunction_v<std::is_nothrow_move_assignable<Ts>...>)
        {
            if ((this == &rhs) || (ValuelessByException() && rhs.ValuelessByException()))
            {
                return *this;
            }

            if (!ValuelessByException() && rhs.ValuelessByException())
            {
                Destroy<0>();
                index_ = variant_npos;
            }
            else
            {
                MoveAssign<0>(std::move(rhs));
            }

            return *this;
        }

        constexpr ~Variant() noexcept { Destroy<0>(); }

        constexpr bool ValuelessByException() const noexcept { return index_ == variant_npos; }

        template <typename Self>
        constexpr std::size_t Index(this Self&& self)
        {
            return std::forward<Self>(self).index_;
        }

        template <std::size_t Index, typename... Args>
        constexpr VariantAlternativeType<Index, Variant<Ts...>>& Emplace(Args&&... args)
        {
            using Type = utilities::GetTypeByIndex<Index, Ts...>;

            Destroy<0>();
            Construct<Type>(std::forward<Args>(args)...);

            index_ = Index;
            return *Cast<Type>();
        }

        template <typename T, typename... Args>
        constexpr T& Emplace(Args&&... args)
        {
            constexpr auto index = utilities::GetTypeIndex<T, Ts...>;  // NOLINT
            return Emplace<index>(std::forward<Args>(args)...);
        }

    private:  // NOLINT -> for readability
        template <typename T>
        constexpr T* Cast()
        {
            // NOLINTNEXTLINE -> reinterpret_cast using
            return std::launder(reinterpret_cast<T*>(std::data(storage_)));
        }

        template <typename T>
        constexpr const T* ConstCast() const
        {
            // NOLINTNEXTLINE -> reinterpret_cast using
            return std::launder(reinterpret_cast<const T*>(std::data(storage_)));
        }

        template <std::size_t Index>
        constexpr void Destroy() noexcept
        {
            if (Index == index_)
            {
                using Type = utilities::GetTypeByIndex<Index, Ts...>;
                auto* ptr  = Cast<Type>();

                ptr->~Type();
            }
            else
            {
                Destroy<Index + 1>();
            }
        }

        template <>
        constexpr void Destroy<sizeof...(Ts)>() noexcept
        {
        }

        template <typename T, typename... Args>
        constexpr void Construct(Args&&... args)
        {
            try
            {
                new (std::data(storage_)) T(std::forward<Args>(args)...);
            }
            catch (...)
            {
                index_ = variant_npos;
                throw;
            }
        }

        template <std::size_t Index>
        constexpr void CopyConstruct(const Variant& rhs)
        {
            if (Index != rhs.index_)
            {
                CopyConstruct<Index + 1>(rhs);
            }
            else
            {
                using Type = utilities::GetTypeByIndex<Index, Ts...>;
                new (std::data(storage_)) Type(Get<Index>(rhs));
            }
        }

        template <>
        constexpr void CopyConstruct<sizeof...(Ts)>([[maybe_unused]] const Variant& rhs)
        {
        }

        template <std::size_t Index>
        constexpr void MoveConstruct(Variant<Ts...>&& rhs) noexcept
        {
            if (Index != rhs.index_)
            {
                MoveConstruct<Index + 1>(std::move(rhs));
            }
            else
            {
                using Type = utilities::GetTypeByIndex<Index, Ts...>;
                new (std::data(storage_)) Type(Get<Index>(std::move(rhs)));
            }
        }

        template <>
        constexpr void MoveConstruct<sizeof...(Ts)>([[maybe_unused]] Variant<Ts...>&& rhs) noexcept
        {
        }

        template <std::size_t Index>
        constexpr void CopyAssign(const Variant<Ts...>& rhs)
        {
            if (Index != rhs.index_)
            {
                CopyAssign<Index + 1>(rhs);
            }
            else
            {
                Get<Index>(*this) = Get<Index>(rhs);
            }
        }

        template <>
        constexpr void CopyAssign<sizeof...(Ts)>([[maybe_unused]] const Variant<Ts...>& rhs)
        {
        }

        template <std::size_t Index>
        constexpr void MoveAssign(Variant<Ts...>&& rhs) noexcept
        {
            if (Index != rhs.index_)
            {
                MoveAssign<Index + 1>(std::move(rhs));
            }
            else
            {
                Get<Index>(*this) = Get<Index>(std::move(rhs));
            }
        }

        template <>
        constexpr void MoveAssign<sizeof...(Ts)>([[maybe_unused]] Variant<Ts...>&& rhs) noexcept
        {
        }

    private:  // NOLINT -> for readability
        std::size_t index_;
        alignas(Ts...) std::array<std::byte, std::max({sizeof(Ts)...})> storage_;
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

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        Variant<Ts...>* variant) noexcept
    {
        if (variant == nullptr || variant->index_ != Index)
        {
            return nullptr;
        }

        using Type = VariantAlternativeType<Index, Variant<Ts...>>;
        return variant->template Cast<Type>();
    }

    template <std::size_t Index, typename... Ts>
    constexpr std::add_pointer_t<const VariantAlternativeType<Index, Variant<Ts...>>> GetIf(
        const Variant<Ts...>* variant) noexcept
    {
        if (variant == nullptr || variant->index_ != Index)
        {
            return nullptr;
        }

        using Type = VariantAlternativeType<Index, Variant<Ts...>>;
        return variant->template ConstCast<Type>();
    }

    template <typename Target, typename... Ts>
    constexpr std::add_pointer_t<Target> GetIf(Variant<Ts...>* variant) noexcept
    {
        return GetIf<utilities::GetTypeIndex<Target, Ts...>>(variant);
    }

    template <typename Target, typename... Ts>
    constexpr std::add_pointer_t<const Target> GetIf(const Variant<Ts...>* variant) noexcept
    {
        return GetIf<utilities::GetTypeIndex<Target, Ts...>>(variant);
    }

    template <std::size_t Index, typename... Ts>
    constexpr VariantAlternativeType<Index, Variant<Ts...>>& Get(Variant<Ts...>& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return *ptr;
    }

    template <std::size_t Index, typename... Ts>
    // NOLINTNEXTLINE -> variant is never moved inside
    constexpr VariantAlternativeType<Index, Variant<Ts...>>&& Get(Variant<Ts...>&& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return std::move(*ptr);
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>& Get(
        const Variant<Ts...>& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return *ptr;
    }

    template <std::size_t Index, typename... Ts>
    constexpr const VariantAlternativeType<Index, Variant<Ts...>>&& Get(
        const Variant<Ts...>&& variant)
    {
        auto* ptr = GetIf<Index>(std::addressof(variant));
        if (!ptr)
        {
            throw BadVariantAccess();
        }

        return std::move(*ptr);
    }

    template <typename T, typename... Ts>
    constexpr T& Get(Variant<Ts...>& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr T&& Get(Variant<Ts...>&& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(std::move(variant));
    }

    template <typename T, typename... Ts>
    constexpr const T& Get(const Variant<Ts...>& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(variant);
    }

    template <typename T, typename... Ts>
    constexpr const T&& Get(const Variant<Ts...>&& variant)
    {
        return Get<utilities::GetTypeIndex<T, Ts...>>(std::move(variant));
    }
}  // namespace variantx
