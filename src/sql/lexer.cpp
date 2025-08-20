//
// Created by Amit Chavan on 7/15/25.
//

/**
 * @file lexer.cpp
 * @brief Implementation of the SQL lexical analyzer
 * 
 * This file implements the Lexer class which converts SQL query strings
 * into sequences of tokens for parsing. The lexer uses a character-by-character
 * scanning approach with lookahead for multi-character operators.
 */

#include "lexer.h"
#include "token_type_utils.h"
#include "spdlog/spdlog.h"
namespace minidb {

    /**
     * @brief Main tokenization method that extracts the next token from input
     * 
     * This method implements a finite state machine that:
     * 1. Skips whitespace characters
     * 2. Recognizes single-character tokens (punctuation, operators)
     * 3. Delegates to specialized methods for complex tokens
     * 4. Handles unexpected characters gracefully
     * 
     * @return Next token in the input stream or EOF_FILE when exhausted
     */
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

            // Handle unexpected characters with error recovery
            return handle_unexpected_character();
        }
        return  {TokenType::EOF_FILE, ""};
    }

   /**
    * @brief Parses comparison and equality operators
    * 
    * Handles both single-character operators (=, <, >) and two-character
    * operators (!=, <=, >=). Uses lookahead to determine if a second
    * character should be consumed.
    * 
    * @return Token representing the parsed operator
    */
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


    /**
     * @brief Helper method to create a token and advance the cursor
     * 
     * @param type The TokenType for the new token
     * @param value The string representation of the token
     * @return Newly created Token with the specified type and value
     */
    Token Lexer::make_token(TokenType type, const std::string &value) {
        advance(); // Move cursor ahead
        return {type, value};
    }

    /**
     * @brief Parses string literals enclosed in single quotes
     * 
     * Advances past the opening quote, collects characters until the closing
     * quote is found, and returns a STRING_LITERAL token. Currently does not
     * handle escape sequences.
     * 
     * @return Token of type STRING_LITERAL containing the string content
     */
    Token Lexer::make_string() {
        std::string value;
        advance();
        while (!has_ended() && peek() != '\'') {
            value += advance();
        }
        return {TokenType::STRING_LITERAL, value};
    }

    /**
     * @brief Parses integer literal tokens
     * 
     * Collects consecutive digit characters to form an integer literal.
     * Currently only supports positive integers - negative numbers are
     * handled as a unary minus operator followed by a positive integer.
     * 
     * @return Token of type INT_LITERAL with the numeric string
     */
    Token Lexer::make_numbers() {
        std::string number;
        while (!has_ended() && isdigit(peek())) {
            number += advance();
        }
        return {TokenType::INT_LITERAL, number};
    }

    /**
     * @brief Parses SQL keywords and user-defined identifiers
     * 
     * Collects alphanumeric characters and underscores, then performs
     * case-insensitive lookup in the keyword map. If found in the keyword
     * map, returns the corresponding keyword token; otherwise returns an
     * IDENTIFIER token.
     * 
     * @return Token of appropriate keyword type or IDENTIFIER
     */
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

    /**
     * @brief Tokenizes the entire input string into a vector
     * 
     * Repeatedly calls next_token() until EOF_FILE is encountered,
     * collecting all tokens into a vector. The EOF_FILE token is included
     * in the result.
     * 
     * @return Complete vector of tokens from the input string
     */
    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;
        Token token;
        do {
            token = next_token();
            tokens.push_back(token);
        } while (token.type != TokenType::EOF_FILE);
        return tokens;
    }

    /**
     * @brief Error recovery method for unexpected characters
     * 
     * When an unrecognized character is encountered, this method:
     * 1. Advances past the character
     * 2. Logs the occurrence using spdlog
     * 3. Returns an UNKNOWN token containing the character
     * 
     * This allows parsing to continue despite errors.
     * 
     * @return UNKNOWN token containing the unexpected character
     */
    Token Lexer::handle_unexpected_character() {
        char unexpected_char = advance();
        
        // Create UNKNOWN token with the unexpected character
        std::string error_value(1, unexpected_char);

        spdlog::info("Unexpected character {} ", unexpected_char );


        // Optionally, we could add logging here in the future
        // std::cerr << "Unexpected character '" << unexpected_char 
        //           << "' at position " << (curr_pos - 1) << std::endl;
        
        return {TokenType::UNKNOWN, error_value};
    }
}
