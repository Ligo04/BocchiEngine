#pragma once
#include "stdafx.h"
#include "../../libs/vectormath/vectormath.hpp"


static constexpr float M_PI = 3.1415926535897932384626433832795f;
static constexpr float M_PI_OVER_2 = 1.5707963267948966192313216916398f;
static constexpr float M_PI_OVER_4 = 0.78539816339744830961566084581988f;

double MillisecondsNow();
std::string format(const char* format, ...);
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary);
bool SaveFile(const char *name, void const*data, size_t size, bool isbinary);
void Trace(const std::string &str);
void Trace(const char* pFormat, ...);
bool LaunchProcess(const char* commandLine, const char* filenameErr);



class MessageBuffer
{
public:
    MessageBuffer(size_t len) :
        m_dynamic(len > STATIC_LEN ? len : 0),
        m_ptr(len > STATIC_LEN ? m_dynamic.data() : m_static)
    {
    }
    char* Data() { return m_ptr; }

private:
    static const size_t STATIC_LEN = 256;
    char m_static[STATIC_LEN];
    std::vector<char> m_dynamic;
    char* m_ptr;
};

class Log
{
public:
    static int InitLogSystem();
    static int TerminateLogSystem();
    static void Trace(const char* LogString);

private:
    static Log* m_pLogInstance;

    Log();
    virtual ~Log();

    void Write(const char* LogString);

    HANDLE m_FileHandle = INVALID_HANDLE_VALUE;
    #define MAX_INFLIGHT_WRITES 32
    
    OVERLAPPED m_OverlappedData[MAX_INFLIGHT_WRITES];
    uint32_t m_CurrentIOBufferIndex = 0;

    uint32_t m_WriteOffset = 0;
};
