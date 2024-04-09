#include "Runtime/Engine.hpp"

int main()
{
    Bocchi::BocchiEngine *engine = new Bocchi::BocchiEngine();
    engine->StartEngine("");
    engine->Initialize();
    engine->Run();
    engine->Clear();
    engine->ShutdownEngine();
    return 0;
}