// test/test_client.cpp
#include "Client.hpp"
#include "test_utils.hpp"

void test_erase_sendbuf_partial_write()
{
    TEST("eraseSendBuf removes exactly n bytes on partial write");
    Client c;
    c.appendSendBuf("HELLOWORLD");  // 10 bytes
    c.eraseSendBuf(4);              // send()が4バイトだけ返した想定
    ASSERT_EQ(c.getSendBuf(), "OWORLD");
    PASS();
}

void test_erase_sendbuf_full_write()
{
    TEST("eraseSendBuf clears buffer on full write");
    Client c;
    c.appendSendBuf("HELLO");
    c.eraseSendBuf(5);
    ASSERT_TRUE(c.getSendBuf().empty());
    PASS();
}

void test_erase_sendbuf_multiple_partial_writes()
{
    TEST("eraseSendBuf accumulates correctly across repeated partial writes");
    Client c;
    c.appendSendBuf("ABCDEFGHIJ");  // 10 bytes
    c.eraseSendBuf(3);              // "DEFGHIJ" 残る
    c.eraseSendBuf(2);              // "FGHIJ" 残る
    ASSERT_EQ(c.getSendBuf(), "FGHIJ");
    PASS();
}

void run_client_tests()
{
    test_erase_sendbuf_partial_write();
    test_erase_sendbuf_full_write();
    test_erase_sendbuf_multiple_partial_writes();
}