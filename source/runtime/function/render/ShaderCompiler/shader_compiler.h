#pragma once
#include "runtime/core/base/macro.h"
#include "runtime/core/base/hash.h"
#include <map>

namespace bocchi
{
    // hash a string of scource code  and rescur over its #include files
    size_t hashShaderString(const char* p_root_dir, const char* p_shader, size_t result = 2166136261);

    class DefineList : public std::map<const std::string, std::string>
    {
    public:
        bool has(const std::string& str) const { return find(str) != end(); }

        size_t hash(size_t result = HASH_SEED) const
        {
            for (auto it = begin(); it != end(); ++it)
            {
                result = ::hashString(it->first, result);
                result = ::hashString(it->second, result);
            }
            return result;
            ;
        }

        friend DefineList operator+(DefineList& def1, const DefineList& def2)
        {
            for (auto it = def2.begin(); it != def2.end(); ++it)
            {
                def1[it->first] = it->second;
            }
            return def1;
        }
    };
}