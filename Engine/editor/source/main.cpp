#include "runtime/Engine.h"

int main()
{
    Bocchi::BocchiEngine* engine = new Bocchi::BocchiEngine();

    engine->startEngine("");
    engine->initialize();

    engine->run();
    engine->clear();
    engine->shutdownEngine();

    return 1;
}