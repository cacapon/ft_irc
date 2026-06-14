// test/test_main.cpp
#include "test_utils.hpp"

int g_passed = 0;
int g_failed = 0;

void run_parser_tests();
// 新しいテストを増やすたびにここに宣言を追加

int main()
{
    run_parser_tests();

    std::cout << "\n--- " << g_passed << " passed, " << g_failed << " failed ---" << std::endl;
    return g_failed > 0 ? 1 : 0;
}