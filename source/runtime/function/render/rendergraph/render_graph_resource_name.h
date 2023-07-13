#pragma once

#include "runtime/core/base/hash.h"

#ifdef _DEBUG
#define RG_DEBUG 1
#else
#define RG_DEBUG 0
#endif

#if RG_DEBUG
#define RG_RES_NAME(name) RgResourceName(#name, bocchi::crc64(#name))
#define RG_RES_NAME(name, idx) RgResourceName(#name, bocchi::crc64(#name) + #idx)
#define RG_STR_RES_NAME(name) RgResourceName(name) RgResourceName(name, adria::crc64(name))
#define RG_STR_RES_NAME_IDX(name, idx) RgResourceName(name, idx) RgResourceName(name, adria::crc64(name) + (idx))
#else
#define RG_RES_NAME(name) RgResourceName(#name, bocchi::crc64(#name))
#define RG_RES_NAME(name, idx) RgResourceName(#name, bocchi::crc64(#name) + #idx)
#define RG_STR_RES_NAME(name) RgResourceName(name) RgResourceName(name, adria::crc64(name))
#define RG_STR_RES_NAME_IDX(name, idx) RgResourceName(name, idx) RgResourceName(name, adria::crc64(name) + (idx))
#endif

namespace bocchi
{
    struct RenderGraphResourceName
    {
        static constexpr uint64_t invalid_hash = static_cast<uint64_t>(-1);
#ifdef RG_DEBUG
        RenderGraphResourceName() : hash_name(invalid_hash), name("unintialized") {}
        template<size_t N>
        constexpr RenderGraphResourceName(char const* (&name)[N], const uint64_t hash) : hash_name(hash), name(name)
        {}

        explicit operator char const*() const { return name; }
#else
        RenderGraphResourceName() : hash_name(invalid_hash) {}
        template<size_t N>
        constexpr RenderGraphResourceName(char const* (&name)[N], const uint64_t hash) : hash_name(hash), name(name)
        {}

        explicit operator char const*() const { return ""; }
#endif
        uint64_t hash_name;

#ifdef RG_DEBUG
        char const* name;
#endif
    };
    using RgResourceName = RenderGraphResourceName;
    inline bool operator==(const RgResourceName& lhs, const RgResourceName& rhs)
    {
        return lhs.hash_name == rhs.hash_name;
    }
} // namespace bocchi

template<>
struct std::hash<bocchi::RenderGraphResourceName>
{
    size_t operator()(bocchi::RenderGraphResourceName const& name) const noexcept
    {
        return hash<decltype(name.hash_name)>()(name.hash_name);
    }
};
