#pragma once

namespace test_basic
{
    // NOLINTBEGIN
    struct ThrowOnCopy
    {
        explicit ThrowOnCopy(int value) : value_(value) {}

        ThrowOnCopy(const ThrowOnCopy&) { throw 42; }
        ThrowOnCopy& operator=(const ThrowOnCopy&) { throw 42; }

    private:
        [[maybe_unused]] int value_;
    };
    // NOLINTEND
}  // namespace test_basic
