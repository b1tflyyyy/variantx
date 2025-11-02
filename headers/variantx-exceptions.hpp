#pragma once

#include <exception>

namespace variantx
{
    struct BadVariantAccess : std::exception
    {
        // NOLINTNEXTLINE
        const char* what() const noexcept { return "Variant does not hold this value"; }
    };
}  // namespace variantx
