//
// Created by Amit Chavan on 7/15/25.
//

#include "parser.h"


namespace minidb {

    std::unique_ptr<ASTNode> minidb::Parser::parse() {

        std::unique_ptr<ASTNode> rootNode;

        switch (peek().type) {
            case TokenType::SELECT:
                return parse_select_node();
            case TokenType::INSERT:
                //TODO implement parse_insert_node();
            case TokenType::DELETE:
                //TODO implement parse_delete_node();
            case TokenType::UPDATE:
                //TODO implement parse_update_node();
            case TokenType::DROP:
                //TODO implement parse_drop_node();
            case TokenType::CREATE:
                //TODO implement parse_create_node();
        }

    }

    std::unique_ptr<ASTNode> Parser::parse_select_node() {
        auto rootNode = std::make_unique<SelectStatementNode>();
        advance(); // Advance past the first token

        // The second token is either * or list of columns.
        if (match(TokenType::STAR)) {
            advance(); // move to next token
            rootNode->is_select_all;
        } else {
            rootNode->columns = parse_columns_collection();
        }

        ensure(TokenType::FROM, "Expected identifier From.");

        // Now parse user u, person p
        rootNode->from_clauses = parse_from_clauses();


        return rootNode;
    }

    bool Parser::match(TokenType type) {
        return !(is_at_end() || peek().type != type);
    }



    std::vector<SelectStatementNode::SelectColumn> Parser::parse_columns_collection() {
        // Select col, col2, col2
        // Select t.col as xx, t.col2,
        std::vector<SelectStatementNode::SelectColumn> columns;
        do {
            if (peek().type == TokenType::COMMA) {
                advance();
            }

            SelectStatementNode::SelectColumn curr_column;
            curr_column.expression = extract_column();

            if (match(TokenType::AS)) {
                advance();
                curr_column.alias = ensure(TokenType::IDENTIFIER, "Expected alias name.").text;
            }
            columns.push_back(std::move(curr_column));
        } while (match(TokenType::COMMA));
        return columns;
    }

    const Token& Parser::ensure(TokenType type, const std::string& message) {
        if (peek().type == type) return advance();
        throw std::runtime_error(message + " Got token with text: " + peek().text);
    }

    std::unique_ptr<ExpressionNode> Parser::extract_column() {
        if (!match(TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected identifier instead found " + peek().text);
        }

        std::string name = advance().text;
        if (match(TokenType::DOT)) {
            advance();
            std::string member_name = ensure(TokenType::IDENTIFIER, "Expected column name after '.'").text;
            auto table_ident = std::make_unique<IdentifierNode>(name);
            auto column_ident = std::make_unique<IdentifierNode>(member_name);
            return std::make_unique<BinaryOperationNode>(std::move(table_ident), ".", std::move(column_ident));
        } else {
            return std::make_unique<IdentifierNode>(name);
        }
    }

    std::vector<SelectStatementNode::TableReference> Parser::parse_from_clauses() {
        std::vector<SelectStatementNode::TableReference> table_ref;
        do {
            // Parsing tableName alias format
            if (tokens[pos+1].type == TokenType::COMMA) {

            } else {

            }

        } while (peek().type != TokenType::WHERE);
        return table_ref;
    }
}

