#pragma once

#include <cassert>
#include <cstddef>
#include <exception>
#include <vector>

// NOLINTBEGIN
namespace advanced_test
{
    template <typename... Ts>
    struct Overload : Ts...
    {
        using Ts::operator()...;
    };

    template <typename... Ts>
    Overload(Ts...) -> Overload<Ts...>;

    struct Trivial
    {
    };

    struct NoDefaultConstructor
    {
        NoDefaultConstructor() = delete;
    };

    struct NonTrivialDestructor
    {
        inline static size_t destructor_count = 0;

        static void reset_counters() { destructor_count = 0; }

        ~NonTrivialDestructor() { ++destructor_count; }
    };

    struct ConstexprNonTrivialDestructor
    {
        constexpr ~ConstexprNonTrivialDestructor() {}

        int x;
    };

    struct ThrowingDefaultConstructor
    {
        ThrowingDefaultConstructor() { throw std::exception(); }
    };

    struct ThrowingSwap
    {
        friend void swap(ThrowingSwap&, ThrowingSwap&) { throw std::exception(); }
    };

    struct DeletedSwap
    {
        friend void swap(DeletedSwap& lhs, DeletedSwap& rhs) = delete;
    };

    struct ThrowingMoveAssignmentWithoutMoveConstructor
    {
        inline static size_t swap_called = 0;

        ThrowingMoveAssignmentWithoutMoveConstructor() = default;

        ThrowingMoveAssignmentWithoutMoveConstructor(
            ThrowingMoveAssignmentWithoutMoveConstructor&&) noexcept(false)
        {
            throw std::exception();
        }

        ThrowingMoveAssignmentWithoutMoveConstructor& operator=(
            ThrowingMoveAssignmentWithoutMoveConstructor&&) = default;
    };

    void swap(ThrowingMoveAssignmentWithoutMoveConstructor&,
              ThrowingMoveAssignmentWithoutMoveConstructor&) noexcept
    {
        ThrowingMoveAssignmentWithoutMoveConstructor::swap_called += 1;
    }

    struct NoCopy
    {
        NoCopy(const NoCopy&) = delete;
    };

    struct NoMove
    {
        NoMove(NoMove&&) = delete;
    };

    struct NonTrivialCopy
    {
        explicit NonTrivialCopy(int x) noexcept : x{x} {}

        NonTrivialCopy(const NonTrivialCopy& other) noexcept : x{other.x + 1} {}

        int x;
    };

    struct MoveConstructorWithoutMoveAssignment
    {
        MoveConstructorWithoutMoveAssignment() = default;

        MoveConstructorWithoutMoveAssignment(MoveConstructorWithoutMoveAssignment&& other) noexcept
            : x(other.x)
        {
        }

        MoveConstructorWithoutMoveAssignment(int val) noexcept : x(val) {}

        MoveConstructorWithoutMoveAssignment(const MoveConstructorWithoutMoveAssignment& other) =
            delete;
        MoveConstructorWithoutMoveAssignment& operator=(
            const MoveConstructorWithoutMoveAssignment& other) = delete;
        MoveConstructorWithoutMoveAssignment& operator=(
            MoveConstructorWithoutMoveAssignment&& other) = delete;

        friend void swap(MoveConstructorWithoutMoveAssignment& lhs,
                         MoveConstructorWithoutMoveAssignment& rhs) noexcept
        {
            std::swap(lhs.x, rhs.x);
        }

        int x;
    };

    struct NonTrivialCopyAssignment
    {
        static constexpr int CTOR_DELTA   = 5;
        static constexpr int ASSIGN_DELTA = 6;

        explicit NonTrivialCopyAssignment(int x) noexcept : x{x} {}

        NonTrivialCopyAssignment(const NonTrivialCopyAssignment& other) noexcept
            : x(other.x + CTOR_DELTA)
        {
        }

        NonTrivialCopyAssignment& operator=(const NonTrivialCopyAssignment& other) noexcept
        {
            if (this != &other)
            {
                x = other.x + ASSIGN_DELTA;
            }
            return *this;
        }

        int x;
    };

    struct NonTrivialCopyWithTrivialMove
    {
        NonTrivialCopyWithTrivialMove(const NonTrivialCopyWithTrivialMove&) {}

        NonTrivialCopyWithTrivialMove(NonTrivialCopyWithTrivialMove&&) = default;

        NonTrivialCopyWithTrivialMove& operator=(const NonTrivialCopyWithTrivialMove&)
        {
            return *this;
        }

        NonTrivialCopyWithTrivialMove& operator=(NonTrivialCopyWithTrivialMove&&) = default;
    };

    struct NonTrivialIntWrapper
    {
        NonTrivialIntWrapper(int x) : x{x} {}

        NonTrivialIntWrapper& operator=(int i)
        {
            x = i + 1;
            return *this;
        }

        friend constexpr bool operator==(const NonTrivialIntWrapper& lhs,
                                         const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x == rhs.x;
        }

        friend constexpr bool operator!=(const NonTrivialIntWrapper& lhs,
                                         const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x != rhs.x;
        }

        friend constexpr bool operator<(const NonTrivialIntWrapper& lhs,
                                        const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x < rhs.x;
        }

        friend constexpr bool operator<=(const NonTrivialIntWrapper& lhs,
                                         const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x <= rhs.x;
        }

        friend constexpr bool operator>(const NonTrivialIntWrapper& lhs,
                                        const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x > rhs.x;
        }

        friend constexpr bool operator>=(const NonTrivialIntWrapper& lhs,
                                         const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x >= rhs.x;
        }

        friend constexpr auto operator<=>(const NonTrivialIntWrapper& lhs,
                                          const NonTrivialIntWrapper& rhs) noexcept
        {
            return lhs.x <=> rhs.x;
        }

        int x;
    };

    struct NoMoveAssignment
    {
        NoMoveAssignment& operator=(NoMoveAssignment&&) = delete;
    };

    struct NoCopyAssignment
    {
        NoCopyAssignment& operator=(const NoCopyAssignment&) = delete;
    };

    struct ThrowingMoveAssignment
    {
        ThrowingMoveAssignment(ThrowingMoveAssignment&&) = default;

        ThrowingMoveAssignment& operator=(ThrowingMoveAssignment&&) noexcept(false)
        {
            throw std::exception();
        }
    };

    struct OnlyMovable
    {
        inline static size_t move_assignment_called = 0;

        constexpr OnlyMovable() = default;

        constexpr OnlyMovable(OnlyMovable&& other) noexcept
        {
            assert(other.coin && "Move of moved value?");
            coin       = true;
            other.coin = false;
        }

        constexpr OnlyMovable& operator=(OnlyMovable&& other) noexcept
        {
            if (this != &other)
            {
                assert(other.coin && "Move of moved value?");
                move_assignment_called += 1;
                coin       = true;
                other.coin = false;
            }
            return *this;
        }

        [[nodiscard]] constexpr bool has_coin() const noexcept { return coin; }

        OnlyMovable(const OnlyMovable&)            = delete;
        OnlyMovable& operator=(const OnlyMovable&) = delete;

    private:
        bool coin{true};
    };

    struct Coin
    {
        constexpr operator int() noexcept { return 42; }
    };

    struct CoinWrapper
    {
        constexpr CoinWrapper() noexcept = default;

        constexpr CoinWrapper(CoinWrapper&& other) noexcept
        {
            assert(other.coin && "Move of moved value?");
            coin       = other.coin;
            other.coin = 0;
        }

        constexpr CoinWrapper& operator=(CoinWrapper&& other) noexcept
        {
            if (this != &other)
            {
                assert((other.coin != 0) && "Move of moved value?");
                coin       = other.coin;
                other.coin = 0;
            }
            return *this;
        }

        [[nodiscard]] constexpr auto has_coins() const noexcept { return coin; }

        constexpr explicit CoinWrapper(Coin) noexcept : coin{17} {}

        constexpr CoinWrapper(const CoinWrapper& other) noexcept : coin(other.coin + 1) {}

        constexpr CoinWrapper& operator=(const CoinWrapper& other) noexcept
        {
            if (this != &other)
            {
                coin = other.coin + 1;
            }
            return *this;
        }

    private:
        int coin{1};
    };

    struct SumOfSquaresVisitor
    {
        template <typename... Args>
        constexpr long operator()(Args... args) const noexcept
        {
            return ((args * args) + ...);
        }
    };

    struct StrangeVisitor
    {
        StrangeVisitor() = default;

        StrangeVisitor(const StrangeVisitor&)            = delete;
        StrangeVisitor(StrangeVisitor&&)                 = delete;
        StrangeVisitor& operator=(const StrangeVisitor&) = delete;
        StrangeVisitor& operator=(StrangeVisitor&&)      = delete;

        constexpr int operator()(int value) & { return value; }

        constexpr int operator()(int value) && { return value + 1; }

        constexpr int operator()(int value) const& { return value + 2; }

        constexpr int operator()(int value) const&& { return value + 3; }
    };

    struct OverloadedAddressOf
    {
        OverloadedAddressOf* operator&() { return nullptr; }

        const OverloadedAddressOf* operator&() const { return nullptr; }

        std::vector<int> x;
    };

    struct EmptyComparable
    {
        EmptyComparable() = default;

        EmptyComparable(EmptyComparable&&) { throw std::exception(); }

        EmptyComparable& operator=(EmptyComparable&&) { throw std::exception(); }

        bool operator==(const EmptyComparable&) const { throw std::exception(); }

        bool operator!=(const EmptyComparable&) const { throw std::exception(); }

        bool operator<(const EmptyComparable&) const { throw std::exception(); }

        bool operator<=(const EmptyComparable&) const { throw std::exception(); }

        bool operator>(const EmptyComparable&) const { throw std::exception(); }

        bool operator>=(const EmptyComparable&) const { throw std::exception(); }
    };

    struct ComparisonCounters
    {
        size_t equal         = 0;
        size_t not_equal     = 0;
        size_t less          = 0;
        size_t less_equal    = 0;
        size_t greater       = 0;
        size_t greater_equal = 0;
        size_t spaceship     = 0;
    };

    struct CustomComparable
    {
        CustomComparable(int value, ComparisonCounters* counters) : value(value), counters(counters)
        {
        }

        bool operator==(const CustomComparable& other) const
        {
            ++counters->equal;
            return this->value == other.value;
        }

        bool operator!=(const CustomComparable& other) const
        {
            ++counters->not_equal;
            return this->value != other.value;
        }

        bool operator<(const CustomComparable& other) const
        {
            ++counters->less;
            return this->value < other.value;
        }

        bool operator<=(const CustomComparable& other) const
        {
            ++counters->less_equal;
            return this->value <= other.value;
        }

        bool operator>(const CustomComparable& other) const
        {
            ++counters->greater;
            return this->value > other.value;
        }

        bool operator>=(const CustomComparable& other) const
        {
            ++counters->greater_equal;
            return this->value >= other.value;
        }

        auto operator<=>(const CustomComparable& other) const
        {
            ++counters->spaceship;
            return this->value <=> other.value;
        }

    private:
        int                 value;
        ComparisonCounters* counters;
    };

    template <typename Ordering>
    struct CustomOrdered
    {
        bool operator==(const CustomOrdered&) const { return true; }

        Ordering operator<=>(const CustomOrdered&) const noexcept { return Ordering::equivalent; }
    };

    using PartiallyOrdered = CustomOrdered<std::partial_ordering>;
    using WeakOrdered      = CustomOrdered<std::weak_ordering>;
    using StrongOrdered    = CustomOrdered<std::strong_ordering>;

    struct ThrowingMemberParams
    {
        bool throwing_copy            = false;
        bool throwing_move            = false;
        bool throwing_copy_assignment = false;
        bool throwing_move_assignment = false;
    };

    struct ThrowingMembersConstructorTag
    {
    };

    template <ThrowingMemberParams PARAMS>
    struct ThrowingMembers
    {
        inline static size_t copy_ctor_calls;
        inline static size_t move_ctor_calls;
        inline static size_t copy_assignment_calls;
        inline static size_t move_assignment_calls;

        static void reset_counters()
        {
            copy_ctor_calls       = 0;
            move_ctor_calls       = 0;
            copy_assignment_calls = 0;
            move_assignment_calls = 0;
        }

        static size_t copy_calls() { return copy_ctor_calls + copy_assignment_calls; }

        static size_t move_calls() { return move_ctor_calls + move_assignment_calls; }

        ThrowingMembers(ThrowingMembersConstructorTag) noexcept {}

        ThrowingMembers(const ThrowingMembers&) noexcept { ++copy_ctor_calls; }

        ThrowingMembers(const ThrowingMembers&)
            requires(PARAMS.throwing_copy)
        {
            ++copy_ctor_calls;
            throw std::exception();
        }

        ThrowingMembers(ThrowingMembers&&) noexcept { ++move_ctor_calls; }

        ThrowingMembers(ThrowingMembers&&)
            requires(PARAMS.throwing_move)
        {
            ++move_ctor_calls;
            throw std::exception();
        }

        ThrowingMembers& operator=(const ThrowingMembers&) noexcept
        {
            ++copy_assignment_calls;
            return *this;
        }

        ThrowingMembers& operator=(const ThrowingMembers&)
            requires(PARAMS.throwing_copy || PARAMS.throwing_copy_assignment)
        {
            ++copy_assignment_calls;
            throw std::exception();
        }

        ThrowingMembers& operator=(ThrowingMembers&&) noexcept
        {
            ++move_assignment_calls;
            return *this;
        }

        ThrowingMembers& operator=(ThrowingMembers&&)
            requires(PARAMS.throwing_move || PARAMS.throwing_move_assignment)
        {
            ++move_assignment_calls;
            throw std::exception();
        }
    };
}  // namespace advanced_test
// NOLINTEND
