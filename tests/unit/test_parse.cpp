// test/test_parse.cpp
#include "test_utils.hpp"
#include "Commands.hpp"

void test_privmsg_channel()
{
    TEST("PRIVMSG to channel with trailing");
    Message msg = Commands::parseLine("PRIVMSG #general :Hello world");
    ASSERT_EQ(msg.params[0], "#general");
    ASSERT_EQ(msg.params[1], "Hello world");
    PASS();
}

void test_join_multiple_channels()
{
    TEST("JOIN multiple channels");
    Message msg = Commands::parseLine("JOIN #foo #bar");
    ASSERT_EQ(msg.params.size(), (size_t)2);
    PASS();
}

void run_parser_tests()
{
    test_privmsg_channel();
    test_join_multiple_channels();
}