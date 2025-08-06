//
// Created by Amit Chavan on 7/15/25.
//

#pragma once
#include "string"
#include "lexer.h"
#include "token.h"
#include "ast.h"

namespace minidb {

    class Parser {
        public:
            explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), pos(0) {
            }

            std::unique_ptr<ASTNode> parse();

        private:
            std::vector<Token> tokens;

            inline Token& advance() {
                if (!is_at_end()) {
                    return tokens[pos++];
                }
                throw std::out_of_range("Cannot advance past the end of tokens.");
            }

            inline Token& peek() {
                return tokens[pos];
            }

            inline bool is_at_end() {
                return pos >= tokens.size();
            }

            const Token& ensure(TokenType type, const std::string& message);
            std::unique_ptr<ExpressionNode> extract_column();

            // Operations for constructing Select AST Node
            std::unique_ptr<ASTNode> parse_select_node();
            std::vector<SelectStatementNode::SelectColumn> parse_columns_collection();
            std::vector<SelectStatementNode::TableReference> parse_from_clauses();



            std::unique_ptr<ASTNode> parse_insert_node();
            std::unique_ptr<ASTNode> parse_delete_node();
            std::unique_ptr<ASTNode> parse_update_node();
            std::unique_ptr<ASTNode> parse_create_node();
            std::unique_ptr<ASTNode> parse_drop_node();
            bool match(const TokenType type);
            int pos;
    };
}


