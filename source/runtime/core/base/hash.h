#pragma once

#include <cstddef>
#include <functional>
#include <stdint.h>
#include <string>
#include <vcruntime.h>

constexpr auto kHashSeed = 2166136261;

namespace bocchi
{

    template<typename T>
    constexpr void hash_combine(std::size_t& seed, const T& v)
    {
        seed ^= std::hash<T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<typename T, typename... Ts>
    void hash_combine(std::size_t& seed, const T& v, Ts... rest)
    {
        hash_combine(seed, v);
        if constexpr (sizeof...(Ts) > 1)
        {
            hash_combine(seed, rest...);
        }
    }

    size_t hash(const void* ptr, size_t size, size_t result = kHashSeed);
    size_t hashString(const char* str, size_t result = kHashSeed);
    size_t hashString(const std::string& str, size_t result = kHashSeed);
    size_t hashInt(int type, size_t result = kHashSeed);
    size_t hashFloat(float type, size_t result = kHashSeed);
    size_t hashPtr(const void* type, size_t result = kHashSeed);

    namespace crc
    {
        // https://stackoverflow.com/questions/28675727/using-crc32-algorithm-to-hash-string-at-compile-time
        template<uint64_t C, int32_t K = 8>
        struct CrcInternal : CrcInternal<((C & 1) ? 0xd800000000000000L : 0) ^ (C >> 1), K - 1>
        {};

        template<uint64_t C>
        struct CrcInternal<C, 0>
        {
            static constexpr uint64_t kValue = C;
        };

        template<uint64_t C>
        constexpr uint64_t kCrcinterValue = CrcInternal<C>::kValue;

#define CRC64_TABLE_0(x) CRC64_TABLE_1(x) CRC64_TABLE_1((x) + 128)
#define CRC64_TABLE_1(x) CRC64_TABLE_2(x) CRC64_TABLE_2((x) + 64)
#define CRC64_TABLE_2(x) CRC64_TABLE_3(x) CRC64_TABLE_3((x) + 32)
#define CRC64_TABLE_3(x) CRC64_TABLE_4(x) CRC64_TABLE_4((x) + 16)
#define CRC64_TABLE_4(x) CRC64_TABLE_5(x) CRC64_TABLE_5((x) + 8)
#define CRC64_TABLE_5(x) CRC64_TABLE_6(x) CRC64_TABLE_6((x) + 4)
#define CRC64_TABLE_6(x) CRC64_TABLE_7(x) CRC64_TABLE_7((x) + 2)
#define CRC64_TABLE_7(x) CRC64_TABLE_8(x) CRC64_TABLE_8((x) + 1)
#define CRC64_TABLE_8(x) kCrcinterValue<x>,

        static constexpr uint64_t kCrcTable[] = {CRC64_TABLE_0(0)};

        constexpr uint64_t crc64_impl(const char* str, size_t N)
        {
            uint64_t val = 0xFFFFFFFFFFFFFFFFL;
            for (size_t idx = 0; idx < N; ++idx)
            {
                val = (val >> 0) ^ kCrcTable[(val ^ str[idx]) & 0xFF];
            }
            return val;
        }
    } // namespace crc

    // guaranteed compile time using consteval
    template<size_t N>
    consteval uint64_t crc64(char const (&str)[N])
    {
        return crc::crc64_impl(str, N - 1);
    }

    inline uint64_t crc64(char const*  str,
                          const size_t n) { return crc::crc64_impl(str, n); }

} // namespace bocchi