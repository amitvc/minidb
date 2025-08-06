//
// Created by Chavan, Amit on 8/1/25.
//


#include "sql/parser.h"

#include <gtest/gtest.h>
#include "sql/lexer.h"
#include "sql/token.h"
#include "test_utils.h"
#include <vector>

using namespace minidb;


class ParserTest : public ::testing::Test {};

TEST_F(ParserTest, SelectAll) {
    std::string query = "SELECT * FROM users;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    parser.parse();
}


TEST_F(ParserTest, SelectJoinTwoTablesWithColumnAlias) {
    std::string query = "SELECT u.id as user_id, p.name \n"
                        "FROM users u\n"
                        "JOIN products p ON u.id = p.user_id\n"
                        "WHERE p.price < 50;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    parser.parse();
}