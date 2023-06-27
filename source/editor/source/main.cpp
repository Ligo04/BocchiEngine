#include "runtime/Engine.h"
#include <nvrhi/nvrhi.h>
int main()
{
    bocchi::BocchiEngine* engine = new bocchi::BocchiEngine();

    engine->startEngine("");
    engine->initialize();

    engine->run();
    engine->clear();
    engine->shutdownEngine();

    return 1;
}