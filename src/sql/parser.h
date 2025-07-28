//
// Created by Amit Chavan on 7/15/25.
//

#pragma once
#include "string"
#include "lexer.h"
#include "token.h"
#include "sql_command.h"

namespace minidb {

    class Parser {
        public:
            Parser(const std::string &input) : lexer_(input) {}
            std::unique_ptr<ASTNode> parse();

        private:
            Lexer lexer_;

            Token get_next_token();
            bool match(TokenType type);
            Token expect(TokenType type);
            std::unique_ptr<SelectNode> parse_select();
            std::vector<std::string> parse_column_list(Token initial_token = {});
    };
}


