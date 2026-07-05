// test/test_parse.cpp
#include "Commands.hpp"
#include "test_utils.hpp"

void test_privmsg_channel()
{
    TEST("PRIVMSG to channel with trailing");
    Message msg = Commands::parseLine("PRIVMSG #general :Hello world");
    ASSERT_EQ(msg.params[0], "#general");
    ASSERT_EQ(msg.params[1], "Hello world");
    PASS();
}

void test_privmsg_to_nick()
{
    TEST("PRIVMSG to nickname");
    Message msg = Commands::parseLine("PRIVMSG Alice :Hi there");
    ASSERT_EQ(msg.params[0], "Alice");
    ASSERT_EQ(msg.params[1], "Hi there");
    PASS();
}

void test_privmsg_no_recipient()
{
    TEST("PRIVMSG with no params (ERR_NORECIPIENT case)");
    Message msg = Commands::parseLine("PRIVMSG");
    ASSERT_EQ(msg.params.size(), (size_t)0);
    PASS();
}

void test_privmsg_no_text_without_colon()
{
    TEST("PRIVMSG with target but no text (ERR_NOTEXTTOSEND case)");
    Message msg = Commands::parseLine("PRIVMSG Alice");
    ASSERT_EQ(msg.params.size(), (size_t)1);
    PASS();
}

void test_privmsg_no_text_with_empty_colon()
{
    TEST("PRIVMSG with target and empty trailing text (ERR_NOTEXTTOSEND case)");
    Message msg = Commands::parseLine("PRIVMSG #general :");
    ASSERT_EQ(msg.params.size(), (size_t)2);
    ASSERT_EQ(msg.params[1], "");
    PASS();
}

void test_join_multiple_channels()
{
    TEST("JOIN multiple channels");
    Message msg = Commands::parseLine("JOIN #foo #bar");
    ASSERT_EQ(msg.params.size(), (size_t)2);
    PASS();
}

void test_topic_view_only()
{
    TEST("TOPIC with channel only (view request)");
    Message msg = Commands::parseLine("TOPIC #general");
    ASSERT_EQ(msg.params.size(), (size_t)1);
    ASSERT_EQ(msg.params[0], "#general");
    PASS();
}

void test_topic_set()
{
    TEST("TOPIC with channel and trailing topic text");
    Message msg = Commands::parseLine("TOPIC #general :new topic here");
    ASSERT_EQ(msg.params.size(), (size_t)2);
    ASSERT_EQ(msg.params[0], "#general");
    ASSERT_EQ(msg.params[1], "new topic here");
    PASS();
}

void test_topic_clear()
{
    TEST("TOPIC with empty trailing text (clear topic case)");
    Message msg = Commands::parseLine("TOPIC #general :");
    ASSERT_EQ(msg.params.size(), (size_t)2);
    ASSERT_EQ(msg.params[1], "");
    PASS();
}

void test_topic_no_params()
{
    TEST("TOPIC with no params (ERR_NEEDMOREPARAMS case)");
    Message msg = Commands::parseLine("TOPIC");
    ASSERT_EQ(msg.params.size(), (size_t)0);
    PASS();
}

void run_parser_tests()
{
    test_privmsg_channel();
    test_privmsg_to_nick();
    test_privmsg_no_recipient();
    test_privmsg_no_text_without_colon();
    test_privmsg_no_text_with_empty_colon();
    test_join_multiple_channels();
    test_topic_view_only();
    test_topic_set();
    test_topic_clear();
    test_topic_no_params();
}
