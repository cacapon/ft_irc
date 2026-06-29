// test/test_main.cpp
#include "test_utils.hpp"

int g_passed = 0;
int g_failed = 0;

void run_parser_tests();
void run_replies_tests();
// Add a declaration here every time you add a new test

int main()
{
    run_parser_tests();
    run_replies_tests();

    std::cout << "\n--- " << g_passed << " passed, " << g_failed << " failed ---" << std::endl;
    return g_failed > 0 ? 1 : 0;
}