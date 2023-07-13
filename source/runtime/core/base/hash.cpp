#include "hash.h"

namespace bocchi
{
    size_t hash(const void* ptr, size_t size, size_t result)
    {
        for (size_t i = 0; i < size; ++i)
        {
            result = (result + 16777639) ^ static_cast<const char*>(ptr)[i];
        }
        return result;
    }

    size_t hashString(const char* str, size_t result) { return hash(str, strlen(str), result); }

    size_t hashString(const std::string& str, size_t result) { return hash(str.c_str(), result); }

    size_t hashInt(int type, size_t result) { return hash(&type, sizeof(int), result); }

    size_t hashFloat(float type, size_t result) { return hash(&type, sizeof(float), result); }

    size_t hashPtr(const void* type, size_t result) { return hash(&type, sizeof(void*), result); }
} // namespace bocchi
