//
// Created by Amit Chavan on 7/15/25.
//

#include "lexer.h"
#include "token_type_utils.h"
namespace minidb {

    Token Lexer::next_token() {
        while (!has_ended()) {
            char c = peek();
            switch (c) {
                case ' ':
                case '\t':
                case '\r':
                case '\n':
                    advance(); // Skip whitespace
                    continue;

                case '*':
                    return make_token(TokenType::STAR, "*");
                case '.':
                    return make_token(TokenType::DOT, ".");
                case ',' :
                    return make_token(TokenType::COMMA, ",");
                case '(' :
                    return make_token(TokenType::LPAREN, "(");
                case ')' :
                    return make_token(TokenType::RPAREN, ")");
                case ';' :
                    return make_token(TokenType::SEMICOLON, ";");
                case '\'' :
                    return make_string();

                case '=':
                case '>':
                case '<':
                case '!':
                    return make_operator();

            }

            if (isdigit(c)) {
                return make_numbers();
            }

            if (std::isalpha(c) || c == '_') {
                return make_key_or_identifier();
            }
        }
        return  {TokenType::EOF_FILE, ""};
    }

   Token Lexer::make_operator() {
        char first_char = advance(); // Consume the first character of the operator

        switch (first_char) {
            case '=':
                return {TokenType::EQ, "="};
            case '!':
                if (!has_ended() && peek() == '=') {
                    advance(); // Consume the '='
                    return {TokenType::NE, "!="};
                }
                break; // Fallthrough to UNKNOWN for a lone '!'
            case '<':
                if (!has_ended() && peek() == '=') {
                    advance(); // Consume the '='
                    return {TokenType::LTE, "<="};
                }
                return {TokenType::LT, "<"};
            case '>':
                if (!has_ended() && peek() == '=') {
                    advance(); // Consume the '='
                    return {TokenType::GTE, ">="};
                }
                return {TokenType::GT, ">"};
        }

        // If we get here, it's an operator we don't recognize.
        return {TokenType::UNKNOWN, std::string(1, first_char)};
    }


    Token Lexer::make_token(TokenType type, const std::string &value) {
        advance(); // Move cursor ahead
        return {type, value};
    }

    Token Lexer::make_string() {
        std::string value;
        advance();
        while (!has_ended() && peek() != '\'') {
            value += advance();
        }
        return {TokenType::STRING_LITERAL, value};
    }

    Token Lexer::make_numbers() {
        std::string number;
        while (!has_ended() && isdigit(peek())) {
            number += advance();
        }
        return {TokenType::INT_LITERAL, number};
    }

    Token Lexer::make_key_or_identifier() {
        std::string text;
        while (!has_ended() && (std::isalnum(peek()) || peek() == '_')) {
            text += advance();
        }

        std::string upper_text = text;
        std::transform(upper_text.begin(), upper_text.end(), upper_text.begin(),
                       [](unsigned char c){ return std::toupper(c); });


        auto it = keyword_map().find(upper_text);
        if (it != keyword_map().end()) {
            return {it->second, text};
        }

        return {TokenType::IDENTIFIER, text};
    }

    char Lexer::advance() {
        if (has_ended()) return '\0';
        return input_[curr_pos++];
    }

    char Lexer::peek() const {
        if (!has_ended()) {
            return input_[curr_pos];
        }
        return '\0'; // return end of string char.
    }

    bool Lexer::has_ended() const {
        return curr_pos >= input_.size();
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        Token token;
        do {
            token = next_token();
            tokens.push_back(token);
        } while (token.type != TokenType::EOF_FILE);
        return tokens;
    }
}
