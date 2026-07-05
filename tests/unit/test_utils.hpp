// test/test_utils.hpp
#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <sstream>
#include <string>

extern int g_passed;
extern int g_failed;

#define TEST(name) std::cout << "[TEST] " << name << " ... "
#define PASS()                          \
    do                                  \
    {                                   \
        std::cout << "OK" << std::endl; \
        g_passed++;                     \
    } while (0)
#define FAIL(msg)                              \
    do                                         \
    {                                          \
        std::cout << "FAIL" << std::endl;      \
        std::cerr << "  " << msg << std::endl; \
        g_failed++;                            \
    } while (0)

#define ASSERT_EQ(a, b)                                    \
    if ((a) != (b))                                        \
    {                                                      \
        std::ostringstream oss;                            \
        oss << "Expected: " << (b) << "  Actual: " << (a); \
        FAIL(oss.str());                                   \
        return;                                            \
    }

#define ASSERT_TRUE(cond)                 \
    if (!(cond))                          \
    {                                     \
        FAIL("Expected true, got false"); \
        return;                           \
    }

#endif
