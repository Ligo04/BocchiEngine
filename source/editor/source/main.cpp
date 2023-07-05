#include <ostream>

#include "runtime/Engine.h"
#include <UGM/UGM.hpp>
int main()
{
    Ubpa::matf4::eye().print();
    bocchi::BocchiEngine* engine = new bocchi::BocchiEngine();

    engine->StartEngine("");
    engine->Initialize();

    engine->run();
    engine->clear();
    engine->ShutdownEngine();

    return 1;
}
