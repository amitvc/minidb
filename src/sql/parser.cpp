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
                break;
            case TokenType::DELETE:
                //TODO implement parse_delete_node();
                break;
            case TokenType::UPDATE:
                //TODO implement parse_update_node();
                break;
            case TokenType::DROP:
                //TODO implement parse_drop_node();
                break;
            case TokenType::CREATE:
                //TODO implement parse_create_node();
                break;
            default:
                throw std::runtime_error("Unsupported statement type: " + peek().text);
        }
        return nullptr; // Should not reach here

    }

    std::unique_ptr<ASTNode> Parser::parse_select_node() {
        auto rootNode = std::make_unique<SelectStatementNode>();
        advance(); // Advance past the first token

        // The second token is either * or list of columns.
        if (match(TokenType::STAR)) {
            advance(); // move to next token
            rootNode->is_select_all = true;
        } else {
            rootNode->columns = parse_columns_collection();
        }

        ensure(TokenType::FROM, "Expected identifier From.");

        rootNode->from_clause = parse_from_table_ref();

        // We will support only single join in the first implementation
        if (match(TokenType::JOIN)) {
            SelectStatementNode::JoinClause join_clause;
            advance();
            join_clause.table = parse_from_table_ref();
            ensure(TokenType::ON, "Expected 'ON' keyword for JOIN clause.");
            join_clause.on_condition = parse_expression();
            rootNode->join_clause.push_back(std::move(join_clause));
        }

        // Parse optional WHERE clause
        if (match(TokenType::WHERE)) {
            advance(); // Consume WHERE token
            rootNode->where_clause = parse_expression();
        }

        return rootNode;
    }

    /**
     * Checks whether the token passed in to the function matches the token at the current position
     * @param type
     * @return true if match is found or else returns false.
     */
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

    /**
     * @brief ensure the current token if it matches the expected type, otherwise throws an error.
     * It will also move the token index 1 position ahead.
     * @param type The expected TokenType.
     * @param message The error message to throw if the token does not match.
     * @return A const reference to the consumed token.
     */
    const Token &Parser::ensure(TokenType type, const std::string &message) {
        if (peek().type == type) return advance();
        throw std::runtime_error(message + " Got token with text: " + peek().text);
    }

    std::unique_ptr<ExpressionNode> Parser::extract_column() {
        if (!match(TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected identifier instead found " + peek().text);
        } else {
            std::string name = advance().text;
            if (match(TokenType::DOT)) {
                auto qualifier = std::make_unique<IdentifierNode>(name);
                advance();
                auto member_name = ensure(TokenType::IDENTIFIER, "Expected column name after '.'").text;
                auto member = std::make_unique<IdentifierNode>(member_name);
                return std::make_unique<QualifiedIdentifierNode>(std::move(qualifier), std::move(member));
            } else {
                return std::make_unique<IdentifierNode>(name);
            }
        }
    }

    // Helper function to parse a single table reference with an optional alias
    std::unique_ptr<SelectStatementNode::TableReference> Parser::parse_from_table_ref() {
        auto table_ref = std::make_unique<SelectStatementNode::TableReference>();
        table_ref->name = std::make_unique<IdentifierNode>(ensure(TokenType::IDENTIFIER, "Expected table name.").text);


        if (match(TokenType::AS)) {
            advance(); // Consume the AS token
            table_ref->alias = ensure(TokenType::IDENTIFIER, "Expected alias for table.").text;
        } else if (peek().type == TokenType::IDENTIFIER) {
            table_ref->alias = advance().text;
        }
        return table_ref;
    }

    /**
     * Parses a logical expression with AND/OR operators
     * Uses recursive descent parsing with proper operator precedence
     *
     * @return A unique ptr for an ExpressionNode that represents the expression
     */
    std::unique_ptr<ExpressionNode> Parser::parse_expression() {
        return parse_or_expression();
    }

    // Parse OR expressions (lowest precedence)
    std::unique_ptr<ExpressionNode> Parser::parse_or_expression() {
        auto left = parse_and_expression();
        
        while (match(TokenType::OR)) {
            advance(); // Consume OR token
            auto right = parse_and_expression();
            left = std::make_unique<BinaryOperationNode>(std::move(left), "OR", std::move(right));
        }
        
        return left;
    }
    
    // Parse AND expressions (higher precedence than OR)
    std::unique_ptr<ExpressionNode> Parser::parse_and_expression() {
        auto left = parse_comparison_expression();
        
        while (match(TokenType::AND)) {
            advance(); // Consume AND token
            auto right = parse_comparison_expression();
            left = std::make_unique<BinaryOperationNode>(std::move(left), "AND", std::move(right));
        }
        
        return left;
    }
    
    // Parse comparison expressions (=, !=, <, >, <=, >=)
    std::unique_ptr<ExpressionNode> Parser::parse_comparison_expression() {
        // Parse left operand (identifier or qualified identifier)
        auto left = extract_column();
        
        // Parse operator - map token type to operator string
        std::string op;
        if (match(TokenType::EQ)) {
            op = "=";
        } else if (match(TokenType::NE)) {
            op = "!=";
        } else if (match(TokenType::GT)) {
            op = ">";
        } else if (match(TokenType::LT)) {
            op = "<";
        } else if (match(TokenType::GTE)) {
            op = ">=";
        } else if (match(TokenType::LTE)) {
            op = "<=";
        } else {
            throw std::runtime_error("Expected comparison operator in expression");
        }
        
        advance(); // Consume the operator token
        
        // Parse right operand - could be identifier, qualified identifier, or literal
        std::unique_ptr<ExpressionNode> right;
        if (match(TokenType::IDENTIFIER)) {
            right = extract_column();
        } else if (match(TokenType::INT_LITERAL)) {
            int64_t value = std::stoll(advance().text);
            right = std::make_unique<LiteralNode>(value);
        } else if (match(TokenType::STRING_LITERAL)) {
            std::string value = advance().text;
            right = std::make_unique<LiteralNode>(value);
        } else if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
            bool value = advance().text == "TRUE";
            right = std::make_unique<LiteralNode>(value);
        } else {
            throw std::runtime_error("Expected identifier or literal in expression");
        }
        
        return std::make_unique<BinaryOperationNode>(std::move(left), std::move(op), std::move(right));
    }

    /**
    * @brief Consumes the current token and returns it, advancing the stream.
    * @return A const reference to the consumed token.
    */
    Token &Parser::advance() {
        if (!is_at_end()) {
            return tokens[pos++];
        }
        throw std::out_of_range("Cannot advance past the end of tokens.");
    }


}

