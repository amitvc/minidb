//
// Created by Amit Chavan on 7/15/25.
//

#pragma once

#include <utility>
#include <vector>
#include "string"
#include "token.h"

namespace minidb {

    /**
     * Responsible for parsing the SQL query and tokenize the input query into tokens.
     * The tokens are then fed into the Parser to form the abstract syntax tree.
     */
    class Lexer {
        public:
            explicit Lexer(std::string  input) : input_(std::move(input)), curr_pos(0) {}
            Token next_token();
            std::vector<Token> tokenize();
        private:
            std::string input_;
            size_t curr_pos;
            std::vector<Token> buffer_;
            char peek() const;
            char advance();
            bool has_ended() const;
            Token make_token(TokenType type, const std::string &value);
            Token make_operator();
            Token make_key_or_identifier();
            Token make_numbers();
            Token make_string();
    };
}