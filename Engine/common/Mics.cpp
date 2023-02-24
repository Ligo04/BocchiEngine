#include "Mics.h"

double MillisecondsNow()
{
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc= QueryPerformanceCounter(&s_frequency);

    double milliseconds=0;

    if(s_use_qpc)
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        milliseconds = double(1000.0*now.QuadPart)/s_frequency.QuadPart;
    }
    else
    {
        milliseconds = double (GetTickCount());
    }

    return milliseconds;
}

std::string format(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#ifndef _MSC_VER
    size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
    MessageBuffer buf(size);
    std::vsnprintf(buf.Data(), size, format, args);
    va_end(args);
    return std::string(buf.Data(), buf.Data() + size - 1); // We don't want the '\0' inside
#else
    const size_t size = (size_t)_vscprintf(format, args) + 1;
    MessageBuffer buf(size);
    vsnprintf_s(buf.Data(), size, _TRUNCATE, format, args);
    va_end(args);
    return std::string(buf.Data(), buf.Data() + size - 1);
#endif
}


Log* Log::m_pLogInstance = nullptr;

int Log::InitLogSystem()
{
    // Create an instance of the log system if non already exists
    if (!m_pLogInstance)
    {
        m_pLogInstance = new Log();
        assert(m_pLogInstance);
        if (m_pLogInstance)
            return 0;
    }

    // Something went wrong
    return -1;
}

int Log::TerminateLogSystem()
{
    if (m_pLogInstance)
    {
        delete m_pLogInstance;
        m_pLogInstance = nullptr;
        return 0;
    }

    // Something went wrong
    return -1;
}

void Log::Trace(const char* LogString)
{
    assert(m_pLogInstance); // Can't do anything without a valid instance
    if (m_pLogInstance)
    {
        m_pLogInstance->Write(LogString);
    }
}

Log::Log()
{
    PWSTR path = NULL;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);

    m_FileHandle = CreateFileW((std::wstring(path)+L"Vulkan.log").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);
    assert(m_FileHandle != INVALID_HANDLE_VALUE);

    // Initialize the overlapped structure for asynchronous write
    for (uint32_t i = 0; i < MAX_INFLIGHT_WRITES; i++)
        m_OverlappedData[i] = { 0 };
}

Log::~Log()
{
    CloseHandle(m_FileHandle);
    m_FileHandle = INVALID_HANDLE_VALUE;
}


void Trace(const std::string &str)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    
    // Output to attached debugger
    OutputDebugStringA(str.c_str());

    // Also log to file
    Log::Trace(str.c_str());
}

void Trace(const char* pFormat, ...)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);

    va_list args;

    // generate formatted string
    va_start(args, pFormat);
    const size_t bufLen = (size_t)_vscprintf(pFormat, args) + 2;
    MessageBuffer buf(bufLen);
    vsnprintf_s(buf.Data(), bufLen, bufLen, pFormat, args);
    va_end(args);
    strcat_s(buf.Data(), bufLen, "\n");

    // Output to attached debugger
    OutputDebugStringA(buf.Data());

    // Also log to file
    Log::Trace(buf.Data());
}

void OverlappedCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    // We never go into an alert state, so this is just to compile
}

void Log::Write(const char* LogString)
{
    OVERLAPPED* pOverlapped = &m_OverlappedData[m_CurrentIOBufferIndex];

    // Make sure any previous write with this overlapped structure has completed
    DWORD numTransferedBytes;
    GetOverlappedResult(m_FileHandle, pOverlapped, &numTransferedBytes, TRUE);  // This will wait on the current thread

    // Apply increments accordingly
    pOverlapped->Offset = m_WriteOffset;
    m_WriteOffset += static_cast<uint32_t>(strlen(LogString));
    
    m_CurrentIOBufferIndex = (++m_CurrentIOBufferIndex % MAX_INFLIGHT_WRITES);  // Wrap when we get to the end

    bool result = WriteFileEx(m_FileHandle, LogString, static_cast<DWORD>(strlen(LogString)), pOverlapped, OverlappedCompletionRoutine);
    assert(result);
}


//
//  Reads a file into a buffer
//
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary)
{
    FILE *file;

    //Open file
    if (fopen_s(&file, name, isbinary ? "rb" : "r") != 0)
    {
        return false;
    }

    //Get file length
    fseek(file, 0, SEEK_END);
    size_t fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // if ascii add one more char to accomodate for the \0
    if (!isbinary)
        fileLen++;

    //Allocate memory
    char *buffer = (char *)malloc(std::max<size_t>(fileLen, 1));
    if (!buffer)
    {
        fclose(file);
        return false;
    }

    //Read file contents into buffer
    size_t bytesRead = 0;
    if(fileLen > 0)
    {
        bytesRead = fread(buffer, 1, fileLen, file);
    }
    fclose(file);

    if (!isbinary)
    {
        buffer[bytesRead] = 0;    
        fileLen = bytesRead;
    }

    *data = buffer;
    if (size != NULL)
        *size = fileLen;

    return true;
}

bool SaveFile(const char *name, void const*data, size_t size, bool isbinary)
{
    FILE *file;
    if (fopen_s(&file, name, isbinary ? "wb" : "w") == 0)
    {
        fwrite(data, size, 1, file);
        fclose(file);
        return true;
    }

    return false;
}

