#include "Engine.hpp"
#include <Luna/Runtime/Runtime.hpp>
namespace Bocchi
{
    void BocchiEngine::StartEngine(const Path &config_file_path)
    {
        luassert_always(Luna::init());
        m_project_path = config_file_path;
        return;
    };
} //namespace Bocchi