#pragma once

namespace test_utilities
{
    // NOLINTBEGIN
    struct ThrowOnCopy
    {
        explicit ThrowOnCopy(int value) : value_(value) {}
        ThrowOnCopy(const ThrowOnCopy&) { throw 42; }

    private:
        int value_;
    };
    // NOLINTEND
}  // namespace test_utilities
