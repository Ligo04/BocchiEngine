#include "shader_compiler.h"

namespace Bocchi
{
    size_t hashShaderString(const char* p_root_dir, const char* p_shader, size_t result)
    {
        result = hash(p_shader, strlen(p_shader), result);

        const char* pch = p_shader;
        while (*pch != 0)
        {
            if (*pch == '/') // parse comments
            {
                pch++;
                if (*pch != 0 && *pch == '/')
                {
                    pch++;
                    while (*pch != 0 && *pch != '\n')
                        pch++;
                }
                else if (*pch != 0 && *pch == '*')
                {
                    pch++;
                    while ((*pch != 0 && *pch != '*') && (*(pch + 1) != 0 && *(pch + 1) != '/'))
                        pch++;
                }
            }
            else if (*pch == '#') // parse #include
            {
                pch++;
                const char include[] = "include";
                int        i         = 0;
                while ((*pch != 0) && *pch == include[i])
                {
                    pch++;
                    i++;
                }

                if (i == strlen(include))
                {
                    while (*pch != 0 && *pch == ' ')
                        pch++;

                    if (*pch != 0 && *pch == '\"')
                    {
                        pch++;
                        const char* p_name = pch;

                        while (*pch != 0 && *pch != '\"')
                            pch++;

                        char include_name[1024];
                        strcpy_s<1024>(include_name, p_root_dir);
                        strncat_s<1024>(include_name, p_name, pch - p_name);

                        pch++;

                        char* p_shader_code = NULL;
                        if (readFile(include_name, &p_shader_code, NULL, false))
                        {
                            result = hashShaderString(p_root_dir, p_shader_code, result);
                            free(p_shader_code);
                        }
                    }
                }
            }
            else
            {
                pch++;
            }
        }

        return result;
    }
}
