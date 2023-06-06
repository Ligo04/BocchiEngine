#pragma once

#include <cstddef>
#include <functional>
#include <string>

#define HASH_SEED 2166136261

template<typename T>
void hash_combine(std::size_t& seed, const T& v)
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


size_t hash(const void* ptr, size_t size, size_t result = HASH_SEED);
size_t hashString(const char* str, size_t result = HASH_SEED);
size_t hashString(const std::string& str, size_t result = HASH_SEED);
size_t hashInt(const int type, size_t result = HASH_SEED);
size_t hashFloat(const float type, size_t result = HASH_SEED);
size_t hashPtr(const void* type, size_t result = HASH_SEED);
