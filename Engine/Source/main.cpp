#include "Runtime/Engine.hpp"

int main()
{
    Bocchi::BocchiEngine *engine = new Bocchi::BocchiEngine();
    engine->StartEngine("");
    engine->Run();
    engine->ShutdownEngine();
    return 0;
}