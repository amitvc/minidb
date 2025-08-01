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
            rootNode->is_select_all;
        } else {
            rootNode->columns = parse_columns_collection();
        }

        return rootNode;
    }

    bool Parser::match(const TokenType type) {
        return peek().type == type;
    }

    std::vector<SelectStatementNode::SelectColumn> Parser::parse_columns_collection() {
        std::vector<SelectStatementNode::SelectColumn> columns;
        
        return columns;
    }
}

