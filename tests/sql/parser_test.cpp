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

TEST_F(ParserTest, ParserTest) {
    std::string query = "SELECT * FROM users;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    parser.parse();

}