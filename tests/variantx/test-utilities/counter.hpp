#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <memory>

namespace test_utilities
{
    namespace detail
    {
        class CounterBlock
        {
            friend struct std::formatter<test_utilities::detail::CounterBlock>;

            // NOLINTNEXTLINE
            enum Get : std::size_t
            {
                CTOR = 0,
                COPY = 1,
                MOVE = 2,
                DTOR = 3
            };

        public:
            CounterBlock() = default;

            void ConstructAdd() { ++data_[Get::CTOR]; }
            void CopyAdd() { ++data_[Get::COPY]; };
            void MoveAdd() { ++data_[Get::MOVE]; };
            void DestructAdd() { ++data_[Get::DTOR]; }

            bool operator==(const CounterBlock& rhs) const noexcept { return data_ == rhs.data_; }
            bool operator!=(const CounterBlock& rhs) const noexcept { return !(*this == rhs); }

        private:
            std::array<std::size_t, 4> data_;
        };
    }  // namespace detail

    class Counter
    {
    public:
        Counter() : counter_block_(std::make_shared<detail::CounterBlock>())
        {
            counter_block_->ConstructAdd();
        }

        Counter(const Counter& rhs) : counter_block_(rhs.counter_block_)
        {
            counter_block_->CopyAdd();
        }

        Counter& operator=(const Counter& rhs)
        {
            if (this != &rhs)
            {
                counter_block_ = rhs.counter_block_;
                counter_block_->CopyAdd();
            }

            return *this;
        }

        Counter(Counter&& rhs) noexcept : counter_block_(std::move(rhs.counter_block_))
        {
            counter_block_->MoveAdd();
        }

        Counter& operator=(Counter&& rhs) noexcept
        {
            if (this != &rhs)
            {
                counter_block_ = std::move(rhs.counter_block_);
                counter_block_->MoveAdd();
            }

            return *this;
        }

        ~Counter() noexcept { counter_block_->DestructAdd(); }

        std::shared_ptr<detail::CounterBlock> GetCounterBlock() { return counter_block_; }

    private:
        std::shared_ptr<detail::CounterBlock> counter_block_;
    };
}  // namespace test_utilities

template <>
struct std::formatter<test_utilities::detail::CounterBlock> : std::formatter<std::string>
{
    auto format(const test_utilities::detail::CounterBlock& counter_block,
                std::format_context&                        ctx) const
    {
        using test_utilities::detail::CounterBlock::Get::COPY;
        using test_utilities::detail::CounterBlock::Get::CTOR;
        using test_utilities::detail::CounterBlock::Get::DTOR;
        using test_utilities::detail::CounterBlock::Get::MOVE;

        // clang-format off
        return std::formatter<std::string>::format(
            std::format("CTOR: {}\nCOPY: {}\nMOVE: {}\nDTOR: {}",
                        counter_block.data_[CTOR],
                        counter_block.data_[COPY],
                        counter_block.data_[MOVE],
                        counter_block.data_[DTOR]),
            ctx);
        // clang-format on
    }
};
