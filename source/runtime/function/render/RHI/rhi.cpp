#include "rhi.h"
#include "runtime/core/base/macro.h"
namespace bocchi
{
    DefaultMessageCallback& DefaultMessageCallback::GetInstance()
    {
        static DefaultMessageCallback instance;
        return instance;
    }

    void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char* message_text)
    {
        switch (severity)
        {
            case nvrhi::MessageSeverity::Info:
                LOG_INFO(message_text)
                break;
            case nvrhi::MessageSeverity::Warning:
                LOG_WARN(message_text)
                break;
            case nvrhi::MessageSeverity::Error:
                LOG_ERROR(message_text)
                break;
            case nvrhi::MessageSeverity::Fatal:
                LOG_FATAL(message_text)
                break;
        }
    }
}



