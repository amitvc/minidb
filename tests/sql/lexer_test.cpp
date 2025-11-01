//
// Created by Amit Chavan on 7/23/25.
//
#include <gtest/gtest.h>
#include "sql/lexer.h"
#include "sql/token.h"
#include "test_utils.h"
#include <vector>

using namespace minidb;

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
    std::string query = "SELECT u.id as user_id, p.name \n"
                        "FROM users u\n"
                        "JOIN products p ON u.id = p.user_id\n"
                        "WHERE p.price < 50;";
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
            {TokenType::JOIN, "JOIN"},
            {TokenType::IDENTIFIER, "products"},
            {TokenType::IDENTIFIER, "p"},
            {TokenType::ON, "ON"},
            {TokenType::IDENTIFIER, "u"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "id"},
            {TokenType::EQ, "="},
            {TokenType::IDENTIFIER, "p"},
            {TokenType::DOT, "."},
            {TokenType::IDENTIFIER, "user_id"},

            {TokenType::WHERE, "WHERE"},


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
            {TokenType::EOF_FILE, ""}
    };
    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, FloatLiteral) {
    std::string query = "SELECT 3.14 FROM products;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::FLOAT_LITERAL, "3.14"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "products"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };

    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, DateLiteral) {
    std::string query = "SELECT '2025-10-31' FROM events;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::DATE_LITERAL, "2025-10-31"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "events"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };

    assert_tokens_equal(tokens, expected_tokens);
}

TEST_F(LexerTest, TimestampLiteral) {
    std::string query = "SELECT '2025-10-31 12:30:00' FROM events;";
    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenize();

    std::vector<Token> expected_tokens = {
            {TokenType::SELECT, "SELECT"},
            {TokenType::TIMESTAMP_LITERAL, "2025-10-31 12:30:00"},
            {TokenType::FROM, "FROM"},
            {TokenType::IDENTIFIER, "events"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::EOF_FILE, ""}
    };

    assert_tokens_equal(tokens, expected_tokens);
}