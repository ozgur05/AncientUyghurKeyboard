// test_main.cpp — entry point. All TEST(...) cases self-register across the
// translation units linked into this executable.
#include "TestFramework.hpp"

int main()
{
    return testing::runAll();
}
