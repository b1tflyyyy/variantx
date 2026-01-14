#pragma once

template <bool CanCopy, bool CanMove>
struct SfinaeCtorBase
{
};

template <>
// NOLINTNEXTLINE
struct SfinaeCtorBase<false, false>
{
    SfinaeCtorBase()                                 = default;
    SfinaeCtorBase(const SfinaeCtorBase&)            = delete;
    SfinaeCtorBase(SfinaeCtorBase&&)                 = delete;
    SfinaeCtorBase& operator=(const SfinaeCtorBase&) = default;
    SfinaeCtorBase& operator=(SfinaeCtorBase&&)      = default;
};

template <>
// NOLINTNEXTLINE
struct SfinaeCtorBase<true, false>
{
    SfinaeCtorBase()                                 = default;
    SfinaeCtorBase(const SfinaeCtorBase&)            = default;
    SfinaeCtorBase(SfinaeCtorBase&&)                 = delete;
    SfinaeCtorBase& operator=(const SfinaeCtorBase&) = default;
    SfinaeCtorBase& operator=(SfinaeCtorBase&&)      = default;
};

template <>
// NOLINTNEXTLINE
struct SfinaeCtorBase<false, true>
{
    SfinaeCtorBase()                                 = default;
    SfinaeCtorBase(const SfinaeCtorBase&)            = delete;
    SfinaeCtorBase(SfinaeCtorBase&&)                 = default;
    SfinaeCtorBase& operator=(const SfinaeCtorBase&) = default;
    SfinaeCtorBase& operator=(SfinaeCtorBase&&)      = default;
};

template <bool CanCopy, bool CanMove>
struct SfinaeAssignBase
{
};

template <>
// NOLINTNEXTLINE
struct SfinaeAssignBase<false, false>
{
    SfinaeAssignBase()                                   = default;
    SfinaeAssignBase(const SfinaeAssignBase&)            = default;
    SfinaeAssignBase(SfinaeAssignBase&&)                 = default;
    SfinaeAssignBase& operator=(const SfinaeAssignBase&) = delete;
    SfinaeAssignBase& operator=(SfinaeAssignBase&&)      = delete;
};

template <>
// NOLINTNEXTLINE
struct SfinaeAssignBase<false, true>
{
    SfinaeAssignBase()                                   = default;
    SfinaeAssignBase(const SfinaeAssignBase&)            = default;
    SfinaeAssignBase(SfinaeAssignBase&&)                 = default;
    SfinaeAssignBase& operator=(const SfinaeAssignBase&) = delete;
    SfinaeAssignBase& operator=(SfinaeAssignBase&&)      = default;
};

template <>
// NOLINTNEXTLINE
struct SfinaeAssignBase<true, false>
{
    SfinaeAssignBase()                                   = default;
    SfinaeAssignBase(const SfinaeAssignBase&)            = default;
    SfinaeAssignBase(SfinaeAssignBase&&)                 = default;
    SfinaeAssignBase& operator=(const SfinaeAssignBase&) = default;
    SfinaeAssignBase& operator=(SfinaeAssignBase&&)      = delete;
};
