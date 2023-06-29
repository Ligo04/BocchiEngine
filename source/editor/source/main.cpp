#include "runtime/Engine.h"
#include <nvrhi/nvrhi.h>


int main()
{
    bocchi::BocchiEngine* engine = new bocchi::BocchiEngine();

    engine->StartEngine("");
    engine->Initialize();

    engine->run();
    engine->clear();
    engine->ShutdownEngine();

    return 1;
}