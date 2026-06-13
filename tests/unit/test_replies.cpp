// test/test_replies.cpp
#include "test_utils.hpp"
#include "Replies.hpp"

void test_rpl_welcome()
{
    TEST("RPL_WELCOME format");
    ASSERT_EQ(Replies::RPL_WELCOME("nick", "user", "host"),
              ":ircserv 001 nick :Welcome to the Internet Relay Network nick!user@host\r\n");
    PASS();
}

void test_rpl_yourhost()
{
    TEST("RPL_YOURHOST format");
    ASSERT_EQ(Replies::RPL_YOURHOST("nick"), ":ircserv 002 nick :Your host is ircserv, running version 1.0\r\n");
    PASS();
}

void test_rpl_created()
{
    TEST("RPL_CREATED format");
    ASSERT_EQ(Replies::RPL_CREATED("nick", "2026-06-13"), ":ircserv 003 nick :This server was created 2026-06-13\r\n");
    PASS();
}

void test_rpl_myinfo()
{
    TEST("RPL_MYINFO format");
    ASSERT_EQ(Replies::RPL_MYINFO("nick"), ":ircserv 004 nick ircserv 1.0 o itkol\r\n");
    PASS();
}

void test_rpl_channelmodeis_without_params()
{
    TEST("RPL_CHANNELMODEIS without mode params");
    ASSERT_EQ(Replies::RPL_CHANNELMODEIS("nick", "#chan", "+i", ""), ":ircserv 324 nick #chan +i\r\n");
    PASS();
}

void test_rpl_channelmodeis_with_params()
{
    TEST("RPL_CHANNELMODEIS with mode params");
    ASSERT_EQ(Replies::RPL_CHANNELMODEIS("nick", "#chan", "+kl", "key 10"), ":ircserv 324 nick #chan +kl key 10\r\n");
    PASS();
}

void test_rpl_notopic()
{
    TEST("RPL_NOTOPIC format");
    ASSERT_EQ(Replies::RPL_NOTOPIC("nick", "#chan"), ":ircserv 331 nick #chan :No topic is set\r\n");
    PASS();
}

void test_rpl_topic()
{
    TEST("RPL_TOPIC format");
    ASSERT_EQ(Replies::RPL_TOPIC("nick", "#chan", "Hello"), ":ircserv 332 nick #chan :Hello\r\n");
    PASS();
}

void test_rpl_inviting()
{
    TEST("RPL_INVITING format");
    ASSERT_EQ(Replies::RPL_INVITING("nick", "#chan", "target"), ":ircserv 341 nick #chan target\r\n");
    PASS();
}

void test_rpl_namreply()
{
    TEST("RPL_NAMREPLY format");
    ASSERT_EQ(Replies::RPL_NAMREPLY("nick", "#chan", "@op user1 user2"), ":ircserv 353 nick = #chan :@op user1 user2\r\n");
    PASS();
}

void test_rpl_endofnames()
{
    TEST("RPL_ENDOFNAMES format");
    ASSERT_EQ(Replies::RPL_ENDOFNAMES("nick", "#chan"), ":ircserv 366 nick #chan :End of NAMES list\r\n");
    PASS();
}

void test_err_nosuchnick()
{
    TEST("ERR_NOSUCHNICK format");
    ASSERT_EQ(Replies::ERR_NOSUCHNICK("nick", "target"), ":ircserv 401 nick target :No such nick/channel\r\n");
    PASS();
}

void test_err_nosuchchannel()
{
    TEST("ERR_NOSUCHCHANNEL format");
    ASSERT_EQ(Replies::ERR_NOSUCHCHANNEL("nick", "#chan"), ":ircserv 403 nick #chan :No such channel\r\n");
    PASS();
}

void test_err_cannotsendtochan()
{
    TEST("ERR_CANNOTSENDTOCHAN format");
    ASSERT_EQ(Replies::ERR_CANNOTSENDTOCHAN("nick", "#chan"), ":ircserv 404 nick #chan :Cannot send to channel\r\n");
    PASS();
}

void test_err_norecipient()
{
    TEST("ERR_NORECIPIENT format");
    ASSERT_EQ(Replies::ERR_NORECIPIENT("nick", "PRIVMSG"), ":ircserv 411 nick :No recipient given (PRIVMSG)\r\n");
    PASS();
}

void test_err_notexttosend()
{
    TEST("ERR_NOTEXTTOSEND format");
    ASSERT_EQ(Replies::ERR_NOTEXTTOSEND("nick"), ":ircserv 412 nick :No text to send\r\n");
    PASS();
}

void test_err_unknowncommand()
{
    TEST("ERR_UNKNOWNCOMMAND format");
    ASSERT_EQ(Replies::ERR_UNKNOWNCOMMAND("nick", "FOO"), ":ircserv 421 nick FOO :Unknown command\r\n");
    PASS();
}

void test_err_nonicknamegiven()
{
    TEST("ERR_NONICKNAMEGIVEN format");
    ASSERT_EQ(Replies::ERR_NONICKNAMEGIVEN("*"), ":ircserv 431 * :No nickname given\r\n");
    PASS();
}

void test_err_erroneusnickname()
{
    TEST("ERR_ERRONEUSNICKNAME format");
    ASSERT_EQ(Replies::ERR_ERRONEUSNICKNAME("*", "bad nick"), ":ircserv 432 * bad nick :Erroneous nickname\r\n");
    PASS();
}

void test_err_nicknameinuse()
{
    TEST("ERR_NICKNAMEINUSE format");
    ASSERT_EQ(Replies::ERR_NICKNAMEINUSE("*", "taken"), ":ircserv 433 * taken :Nickname is already in use\r\n");
    PASS();
}

void test_err_usernotinchannel()
{
    TEST("ERR_USERNOTINCHANNEL format");
    ASSERT_EQ(Replies::ERR_USERNOTINCHANNEL("nick", "target", "#chan"),
              ":ircserv 441 nick target #chan :They aren't on that channel\r\n");
    PASS();
}

void test_err_notonchannel()
{
    TEST("ERR_NOTONCHANNEL format");
    ASSERT_EQ(Replies::ERR_NOTONCHANNEL("nick", "#chan"), ":ircserv 442 nick #chan :You're not on that channel\r\n");
    PASS();
}

void test_err_useronchannel()
{
    TEST("ERR_USERONCHANNEL format");
    ASSERT_EQ(Replies::ERR_USERONCHANNEL("nick", "target", "#chan"),
              ":ircserv 443 nick target #chan :is already on channel\r\n");
    PASS();
}

void test_err_notregistered()
{
    TEST("ERR_NOTREGISTERED format");
    ASSERT_EQ(Replies::ERR_NOTREGISTERED("*"), ":ircserv 451 * :You have not registered\r\n");
    PASS();
}

void test_err_needmoreparams()
{
    TEST("ERR_NEEDMOREPARAMS format");
    ASSERT_EQ(Replies::ERR_NEEDMOREPARAMS("nick", "JOIN"), ":ircserv 461 nick JOIN :Not enough parameters\r\n");
    PASS();
}

void test_err_alreadyregistred()
{
    TEST("ERR_ALREADYREGISTRED format");
    ASSERT_EQ(Replies::ERR_ALREADYREGISTRED("nick"), ":ircserv 462 nick :Unauthorized command (already registered)\r\n");
    PASS();
}

void test_err_passwdmismatch()
{
    TEST("ERR_PASSWDMISMATCH format");
    ASSERT_EQ(Replies::ERR_PASSWDMISMATCH("*"), ":ircserv 464 * :Password incorrect\r\n");
    PASS();
}

void test_err_keyset()
{
    TEST("ERR_KEYSET format");
    ASSERT_EQ(Replies::ERR_KEYSET("nick", "#chan"), ":ircserv 467 nick #chan :Channel key already set\r\n");
    PASS();
}

void test_err_channelisfull()
{
    TEST("ERR_CHANNELISFULL format");
    ASSERT_EQ(Replies::ERR_CHANNELISFULL("nick", "#chan"), ":ircserv 471 nick #chan :Cannot join channel (+l)\r\n");
    PASS();
}

void test_err_unknownmode()
{
    TEST("ERR_UNKNOWNMODE format");
    ASSERT_EQ(Replies::ERR_UNKNOWNMODE("nick", 'z', "#chan"), ":ircserv 472 nick z :is unknown mode char to me for #chan\r\n");
    PASS();
}

void test_err_inviteonlychan()
{
    TEST("ERR_INVITEONLYCHAN format");
    ASSERT_EQ(Replies::ERR_INVITEONLYCHAN("nick", "#chan"), ":ircserv 473 nick #chan :Cannot join channel (+i)\r\n");
    PASS();
}

void test_err_badchannelkey()
{
    TEST("ERR_BADCHANNELKEY format");
    ASSERT_EQ(Replies::ERR_BADCHANNELKEY("nick", "#chan"), ":ircserv 475 nick #chan :Cannot join channel (+k)\r\n");
    PASS();
}

void test_err_chanoprivsneeded()
{
    TEST("ERR_CHANOPRIVSNEEDED format");
    ASSERT_EQ(Replies::ERR_CHANOPRIVSNEEDED("nick", "#chan"), ":ircserv 482 nick #chan :You're not channel operator\r\n");
    PASS();
}

void run_replies_tests()
{
    test_rpl_welcome();
    test_rpl_yourhost();
    test_rpl_created();
    test_rpl_myinfo();
    test_rpl_channelmodeis_without_params();
    test_rpl_channelmodeis_with_params();
    test_rpl_notopic();
    test_rpl_topic();
    test_rpl_inviting();
    test_rpl_namreply();
    test_rpl_endofnames();
    test_err_nosuchnick();
    test_err_nosuchchannel();
    test_err_cannotsendtochan();
    test_err_norecipient();
    test_err_notexttosend();
    test_err_unknowncommand();
    test_err_nonicknamegiven();
    test_err_erroneusnickname();
    test_err_nicknameinuse();
    test_err_usernotinchannel();
    test_err_notonchannel();
    test_err_useronchannel();
    test_err_notregistered();
    test_err_needmoreparams();
    test_err_alreadyregistred();
    test_err_passwdmismatch();
    test_err_keyset();
    test_err_channelisfull();
    test_err_unknownmode();
    test_err_inviteonlychan();
    test_err_badchannelkey();
    test_err_chanoprivsneeded();
}
