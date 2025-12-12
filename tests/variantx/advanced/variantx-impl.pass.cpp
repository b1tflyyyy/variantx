#include <gtest/gtest.h>

#include <headers/variantx.hpp>
#if 0  // TODO: redef...
#include "../utils/advanced/test-classes.hpp"

TEST(traits_impl, static_assertions)
{
    namespace vi = variantx::impl;

    // kDestructibleTrait
    static_assert(vi::Traits<advanced_test::Trivial>::kDestructibleTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::Trivial, int, double>::kDestructibleTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::NonTrivialDestructor>::kDestructibleTrait ==
                  vi::Trait::Available);
    static_assert(vi::Traits<advanced_test::Trivial,
                             advanced_test::NonTrivialDestructor>::kDestructibleTrait ==
                  vi::Trait::Available);
    static_assert(vi::Traits<void>::kDestructibleTrait == vi::Trait::Unavailable);
    static_assert(vi::Traits<advanced_test::Trivial, void>::kDestructibleTrait ==
                  vi::Trait::Unavailable);

    // kCopyConstructibleTrait
    static_assert(vi::Traits<advanced_test::Trivial>::kCopyConstructibleTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::Trivial, int, double>::kCopyConstructibleTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::NonTrivialCopy>::kCopyConstructibleTrait ==
                  vi::Trait::Available);
    static_assert(vi::Traits<advanced_test::Trivial,
                             advanced_test::NonTrivialCopy>::kCopyConstructibleTrait ==
                  vi::Trait::Available);
    static_assert(vi::Traits<advanced_test::NoCopy>::kCopyConstructibleTrait ==
                  vi::Trait::Unavailable);
    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::NoCopy>::kCopyConstructibleTrait ==
        vi::Trait::Unavailable);

    // kMoveConstructibleTrait
    static_assert(vi::Traits<advanced_test::Trivial>::kMoveConstructibleTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::Trivial, int, double>::kMoveConstructibleTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::OnlyMovable>::kMoveConstructibleTrait ==
                  vi::Trait::Available);
    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::OnlyMovable>::kMoveConstructibleTrait ==
        vi::Trait::Available);
    static_assert(vi::Traits<advanced_test::NoMove>::kMoveConstructibleTrait ==
                  vi::Trait::Unavailable);
    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::NoMove>::kMoveConstructibleTrait ==
        vi::Trait::Unavailable);

    // kCopyAssignableTrait
    static_assert(vi::Traits<advanced_test::Trivial>::kCopyAssignableTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::Trivial, int, double>::kCopyAssignableTrait ==
                  vi::Trait::TriviallyAvailable);
    static_assert(vi::Traits<advanced_test::NonTrivialCopyAssignment>::kCopyAssignableTrait ==
                  vi::Trait::Available);
    static_assert(vi::Traits<advanced_test::Trivial,
                             advanced_test::NonTrivialCopyAssignment>::kCopyAssignableTrait ==
                  vi::Trait::Available);
    static_assert(vi::Traits<advanced_test::NoCopyAssignment>::kCopyAssignableTrait ==
                  vi::Trait::Unavailable);
    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::NoCopyAssignment>::kCopyAssignableTrait ==
        vi::Trait::Unavailable);

    static_assert(vi::Traits<advanced_test::NoCopy>::kCopyConstructibleTrait ==
                  vi::Trait::Unavailable);

    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::NoCopy>::kCopyConstructibleTrait ==
        vi::Trait::Unavailable);

    // kMoveAssignableTrait
    static_assert(vi::Traits<advanced_test::Trivial>::kMoveAssignableTrait ==
                  vi::Trait::TriviallyAvailable);

    static_assert(vi::Traits<advanced_test::Trivial, int, double>::kMoveAssignableTrait ==
                  vi::Trait::TriviallyAvailable);

    static_assert(vi::Traits<advanced_test::OnlyMovable>::kMoveAssignableTrait ==
                  vi::Trait::Available);

    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::OnlyMovable>::kMoveAssignableTrait ==
        vi::Trait::Available);

    static_assert(vi::Traits<advanced_test::NoMoveAssignment>::kMoveAssignableTrait ==
                  vi::Trait::Unavailable);
    static_assert(
        vi::Traits<advanced_test::Trivial, advanced_test::NoMoveAssignment>::kMoveAssignableTrait ==
        vi::Trait::Unavailable);

    static_assert(vi::Traits<advanced_test::NoMove>::kMoveAssignableTrait ==
                  vi::Trait::Unavailable);

    static_assert(vi::Traits<advanced_test::Trivial, advanced_test::NoMove>::kMoveAssignableTrait ==
                  vi::Trait::Unavailable);
}
#endif
