//
// Created by Amit Chavan on 7/23/25.
//
#include <gtest/gtest.h>

#include "sql/lexer.h"
#include "sql/token.h"
#include <vector>

using namespace minidb;
// A helper function to compare two vectors of tokens.
// This makes the test assertions cleaner.
void assert_tokens_equal(const std::vector<Token>& actual, const std::vector<Token>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].type, expected[i].type) << "Token " << i << " type mismatch.";
        EXPECT_EQ(actual[i].text, expected[i].text) << "Token " << i << " text mismatch.";
    }
}

class LexerTest : public ::testing::Test {};


TEST_F(LexerTest, SimpleSelectAll) {
    std::string query = "SELECT * FROM users;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::STAR, "*"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "users"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };

    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, SelectWithColumnName) {
    std::string query = "SELECT name,age,sex FROM users;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::IDENTIFIER, "name"},
            {TokenType::COMMA, ","},
            {TokenType::IDENTIFIER, "age"},
            {TokenType::COMMA, ","},
            {TokenType::IDENTIFIER, "sex"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "users"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };
    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, SelectWithColumnAndWhereClauseName) {
    std::string query = "SELECT name,age,sex FROM users where age >= 40;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::IDENTIFIER, "name"},
            {TokenType::COMMA, ","},
            {TokenType::IDENTIFIER, "age"},
            {TokenType::COMMA, ","},
            {TokenType::IDENTIFIER, "sex"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "users"},
            {TokenType::WHERE, "where"},
            {TokenType::IDENTIFIER, "age"},
            {TokenType::GTE, ">="},
            {TokenType::INT_LITERAL, "40"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };
    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, SelectJoinTwoTablesWithColumnAlias) {
    std::string query = "SELECT u.id as user_id, p.name FROM users u, products p WHERE (u.id = p.user_id) AND p.price < 50;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();
    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::IDENTIFIER, "u"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "id"},
            {TokenType::AS, "as"},
            {TokenType::IDENTIFIER, "user_id"},
            {TokenType::COMMA, ","},
            {TokenType::IDENTIFIER, "p"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "name"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "users"},
            {TokenType::IDENTIFIER, "u"},
            {TokenType::COMMA, ","},
            {TokenType::IDENTIFIER, "products"},
            {TokenType::IDENTIFIER, "p"},
            {TokenType::WHERE, "WHERE"},
            {TokenType::LPAREN, "("},
            {TokenType::IDENTIFIER, "u"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "id"},
            {TokenType::EQ, "="},

            {TokenType::IDENTIFIER, "p"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "user_id"},

            {TokenType::RPAREN, ")"},

            {TokenType::AND, "AND"},

            {TokenType::IDENTIFIER, "p"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "price"},

            {TokenType::LT, "<"},
            {TokenType::INT_LITERAL, "50"},

            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };
    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, InsertStatement) {
    std::string query = "INSERT INTO users VALUES (1, 'Alice');";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::INSERT, "INSERT"},
            {TokenType::INTO, "INTO"},
            {TokenType::IDENTIFIER, "users"},
            {TokenType::VALUES, "VALUES"},
            {TokenType::LPAREN, "("},
            {TokenType::INT_LITERAL, "1"},
            {TokenType::COMMA, ","},
            {TokenType::STRING_LITERAL, "Alice"},
            {TokenType::RPAREN, ")"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_TOKEN, ""}
    };
    assert_tokens_equal(tokens, expected_tokens);
}