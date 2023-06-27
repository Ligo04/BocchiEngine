
#include "misc.h"
#include <fstream>

namespace bocchi
{
    class MessageBuffer
    {
    public:
        explicit MessageBuffer(size_t len) :
            m_static {}, m_dynamic(len > STATIC_LEN ? len : 0), m_ptr(len > STATIC_LEN ? m_dynamic.data() : m_static)
        {}
        char* data() { return m_ptr; }

    private:
        static const size_t STATIC_LEN = 256;
        char                m_static[STATIC_LEN];
        std::vector<char>   m_dynamic;
        char*               m_ptr;
    };
    //
    // Formats a string
    //
    std::string format(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
#ifndef _MSC_VER
        size_t        size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
        MessageBuffer buf(size);
        std::vsnprintf(buf.Data(), size, format, args);
        va_end(args);
        return std::string(buf.Data(), buf.Data() + size - 1); // We don't want the '\0' inside
#else
        const size_t  size = (size_t)_vscprintf(format, args) + 1;
        MessageBuffer buf(size);
        vsnprintf_s(buf.data(), size, _TRUNCATE, format, args);
        va_end(args);
        return std::string(buf.data(), buf.data() + size - 1);
#endif
    }

    bool lauchProcess(const char* command_line, const char* file_name_err)
    {
        char cmd_line[1024];
        strcpy_s<1024>(cmd_line, command_line);

        // create a pipe to get possible errors from the process
        //
        HANDLE g_h_child_std_out_rd = NULL;
        HANDLE g_h_child_std_out_wr = NULL;

        SECURITY_ATTRIBUTES sa_attr;
        sa_attr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        sa_attr.bInheritHandle       = TRUE;
        sa_attr.lpSecurityDescriptor = NULL;
        if (!CreatePipe(&g_h_child_std_out_rd, &g_h_child_std_out_wr, &sa_attr, 0))
            return false;

        // launch process
        //
        PROCESS_INFORMATION pi = {};
        STARTUPINFOA        si = {};
        si.cb                  = sizeof(si);
        si.dwFlags             = STARTF_USESTDHANDLES;
        si.hStdError           = g_h_child_std_out_wr;
        si.hStdOutput          = g_h_child_std_out_wr;
        si.wShowWindow         = SW_HIDE;

        if (CreateProcessA(NULL, cmd_line, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(g_h_child_std_out_wr);

            ULONG rc;
            if (GetExitCodeProcess(pi.hProcess, &rc))
            {
                if (rc == 0)
                {
                    DeleteFileA(file_name_err);
                    return true;
                }

                LOG_ERROR(format("*** Process %s returned an error, see %s ***\n\n", command_line, file_name_err));

                // save errors to disk
                std::ofstream ofs(file_name_err, std::ofstream::out);

                for (;;)
                {
                    DWORD dw_read;
                    char  ch_buf[2049];
                    BOOL  b_success = ReadFile(g_h_child_std_out_rd, ch_buf, 2048, &dw_read, NULL);
                    ch_buf[dw_read] = 0;
                    if (!b_success || dw_read == 0)
                        break;

                    LOG_INFO(ch_buf);

                    ofs << ch_buf;
                }

                ofs.close();
            }

            CloseHandle(g_h_child_std_out_rd);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            LOG_ERROR(format("*** Can't launch: %s \n", command_line));
        }

        return false;
    }

    bool readFile(const char* name, char** data, size_t* size, bool isbinary)
    {
        FILE* file;

        // Open file
        if (fopen_s(&file, name, isbinary ? "rb" : "r") != 0)
        {
            return false;
        }

        // Get file length
        fseek(file, 0, SEEK_END);
        size_t file_len = ftell(file);
        fseek(file, 0, SEEK_SET);

        // if ascii add one more char to accomodate for the \0
        if (!isbinary)
            file_len++;

        // Allocate memory
        char* buffer = (char*)malloc(std::max<size_t>(file_len, 1));
        if (!buffer)
        {
            fclose(file);
            return false;
        }

        // Read file contents into buffer
        size_t bytes_read = 0;
        if (file_len > 0)
        {
            bytes_read = fread(buffer, 1, file_len, file);
        }
        fclose(file);

        if (!isbinary)
        {
            buffer[bytes_read] = 0;
            file_len           = bytes_read;
        }

        *data = buffer;
        if (size != NULL)
            *size = file_len;

        return true;
    }

    bool saveFile(const char* name, void const* data, size_t size, bool isbinary)
    {
        FILE* file;
        if (fopen_s(&file, name, isbinary ? "wb" : "w") == 0)
        {
            fwrite(data, size, 1, file);
            fclose(file);
            return true;
        }

        return false;
    }
} // namespace Bocchi
