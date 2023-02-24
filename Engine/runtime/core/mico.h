#pragma once

#include "runtime/core/log_system.h"

#include <chrono>
#include <thread>

#ifdef NDEBUG
#    define ASSERT(statement)
#else
#    define ASSERT(statement) assert(statement)
#endif   // NDEBUG
