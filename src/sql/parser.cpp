/**
 * @file parser.cpp
 * @brief Implementation of the SQL parser that builds Abstract Syntax Tree for each type SQL query command.
 * 
 * This file implements the Parser class which converts token streams into
 * Abstract Syntax Trees using recursive descent parsing. The parser handles
 * SELECT statements with proper operator precedence for expressions.
 * 
 * Created by Amit Chavan on 7/15/25.
 */

#include "parser.h"


namespace minidb {

    /**
     * @brief Main entry point for parsing the token stream
     * 
     * Examines the first token to determine the statement type and delegates
     * to the appropriate parsing method. Currently only SELECT statements are
     * fully implemented.
     * 
     * @return Root ASTNode representing the parsed SQL statement
     * @throws std::runtime_error for unsupported statement types
     */
    std::unique_ptr<ASTNode> Parser::parse() {

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
                return parse_drop_node();
                break;
            case TokenType::CREATE:
                //TODO implement parse_create_node();
                break;
            default:
                throw std::runtime_error("Unsupported statement type: " + peek().text);
        }
        return nullptr; // Should not reach here

    }

    /**
     * @brief Parses a complete SELECT statement with all clauses
     * 
     * Handles the full SELECT statement syntax:
     * SELECT { * | column_list } FROM table_ref [JOIN table_ref ON condition] [WHERE condition]
     * 
     * @return SelectStatementNode containing all parsed components
     */
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
            join_clause.on_condition = parse_logical_expression();
            rootNode->join_clause.push_back(std::move(join_clause));
        }

        // Parse optional WHERE clause
        if (match(TokenType::WHERE)) {
            advance(); // Consume WHERE token
            rootNode->where_clause = parse_logical_expression();
        }

        if (match(TokenType::GROUP)) {
            advance();
            ensure(TokenType::BY, "Expected 'By' keyword after GROUP. Instead found "+ tokens[pos].text);
            rootNode->group_by = parse_group_by_clause();
        }
        return rootNode;
    }

    /**
     * @brief Parses a complete DROP statement.
     * Handles the full DROP statement syntax:
     * DROP IF EXISTS table list
     * @return DropTableStatementNode containing all parsed components
     */
    std::unique_ptr<ASTNode> Parser::parse_drop_node() {
        auto rootNode = std::make_unique<DropTableStatementNode>();
        advance(); // Advance past the first token
        if (match(TokenType::IF)) {
            advance();
            ensure(TokenType::EXISTS, "Expected 'Exists' keyword after IF.");
            rootNode->if_exists = true;
            do {
                rootNode->table_names.push_back(
                    std::make_unique<IdentifierNode>(ensure(TokenType::IDENTIFIER, "Expected table name.").text)
                );
            } while (match(TokenType::COMMA));
        }
        return rootNode;
    }

    std::unique_ptr<SelectStatementNode::GroupByClause> Parser::parse_group_by_clause() {
        auto clause = std::make_unique<SelectStatementNode::GroupByClause>();
        clause->expressions = parse_expression_list();
        if (match(TokenType::HAVING)) {
            advance();
            clause->having_clause = parse_logical_expression();
        }
        return clause;
    }

    std::vector<std::unique_ptr<ExpressionNode>> Parser::parse_expression_list() {
        std::vector<std::unique_ptr<ExpressionNode>> expressions;
        expressions.push_back(parse_logical_expression());
        
        while (match(TokenType::COMMA)) {
            advance(); // consume comma
            expressions.push_back(parse_logical_expression());
        }
        return expressions;
    }

    /**
     * @brief Checks whether the current token matches the specified type
     * 
     * This is a non-consuming peek operation that checks if the current token
     * matches the given type without advancing the parser position.
     * 
     * @param type TokenType to check against current token
     * @return true if current token matches type and we're not at end, false otherwise
     */
    bool Parser::match(TokenType type) {
        return !(is_at_end() || peek().type != type);
    }

    /**
     * @brief Parses a comma-separated list of column expressions with optional aliases
     * 
     * Handles various column formats:
     * - Simple identifiers: column_name
     * - Qualified identifiers: table.column_name
     * - Aliases: column_name AS alias_name or column_name alias_name
     * 
     * @return Vector of SelectColumn structures containing expressions and aliases
     */
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
     * @brief Validates and consumes a token of the expected type
     * 
     * This method enforces that the current token matches the expected type.
     * If it matches, the token is consumed and returned. If not, a descriptive
     * error is thrown.
     * 
     * @param type The expected TokenType
     * @param message Custom error message to include in exception
     * @return Reference to the consumed token
     * @throws std::runtime_error if token type doesn't match expectation
     */
    const Token &Parser::ensure(TokenType type, const std::string &message) {
        if (peek().type == type) return advance();
        throw std::runtime_error(message + " Got token with text: " + peek().text);
    }

    /**
     * @brief Parses column references including qualified identifiers
     * 
     * Handles both simple identifiers (column_name) and qualified identifiers
     * (table.column_name). Creates appropriate AST nodes for each case.
     * 
     * @return IdentifierNode for simple names or QualifiedIdentifierNode for table.column
     * @throws std::runtime_error if current token is not an identifier
     */
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

    /**
     * @brief Parses a table reference with optional alias in FROM or JOIN clauses
     * 
     * Handles various table reference formats:
     * - Simple table name: table_name
     * - Explicit alias: table_name AS alias_name
     * - Implicit alias: table_name alias_name
     * 
     * @return TableReference structure with table name and optional alias
     */
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
     * @brief Entry point for expression parsing with full operator precedence
     * 
     * This method serves as the top-level entry point for parsing expressions.
     * It delegates to parse_or_expression() which implements the lowest precedence
     * level in the operator precedence hierarchy.
     * 
     * Operator precedence (highest to lowest):
     * 1. Comparison operators (=, !=, <, >, <=, >=)
     * 2. AND
     * 3. OR
     * 
     * @return ExpressionNode representing the complete parsed expression
     */
    std::unique_ptr<ExpressionNode> Parser::parse_logical_expression() {
        auto left = parse_relational_expression();

        while (match(TokenType::AND) || match(TokenType::OR)) {
            advance();
            std::string op = tokens[pos - 1].text;
            auto right = parse_relational_expression();
            left = std::make_unique<BinaryOperationNode>(std::move(left), op, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ExpressionNode> Parser::parse_relational_expression() {
        auto left = parse_value_or_identifier();

        while (peek().type == TokenType::EQ || peek().type == TokenType::NE ||
               peek().type == TokenType::LT || peek().type == TokenType::LTE ||
               peek().type == TokenType::GT || peek().type == TokenType::GTE) {
            std::string op = advance().text;
            auto right = parse_value_or_identifier();
            left = std::make_unique<BinaryOperationNode>(std::move(left), op, std::move(right));
        }
        return left;
    }

    std::unique_ptr<ExpressionNode> Parser::parse_value_or_identifier() {
        if (match(TokenType::INT_LITERAL)) {
            advance();
            int64_t val = std::stoll(tokens[pos - 1].text);
            return std::make_unique<LiteralNode>(val);
        }
        if (match(TokenType::STRING_LITERAL)) {
            advance();
            return std::make_unique<LiteralNode>(tokens[pos - 1].text);
        }

        if (match(TokenType::IDENTIFIER)) {
            advance();
            std::string name = tokens[pos - 1].text;
            if (match(TokenType::DOT)) {
                advance();
                auto qualifier = std::make_unique<IdentifierNode>(name);
                auto member_name = ensure(TokenType::IDENTIFIER, "Expected column name after '.'").text;
                auto member = std::make_unique<IdentifierNode>(member_name);
                return std::make_unique<QualifiedIdentifierNode>(std::move(qualifier), std::move(member));
            }
            return std::make_unique<IdentifierNode>(name);
        }

        if (match(TokenType::LPAREN)) {
            advance();
            auto expr = parse_logical_expression();
            ensure(TokenType::RPAREN, "Expected ')' after expression.");
            return expr;
        }

        throw std::runtime_error("Unexpected token in expression: " + peek().text);
    }


    /**
     * @brief Consumes the current token and advances the parser position
     * 
     * This method moves the parser to the next token in the stream and returns
     * a reference to the token that was consumed. It includes bounds checking
     * to prevent advancing past the end of the token stream.
     * 
     * @return Reference to the consumed token
     * @throws std::out_of_range if attempting to advance past end of tokens
     */
    Token &Parser::advance() {
        if (!is_at_end()) {
            return tokens[pos++];
        }
        throw std::out_of_range("Cannot advance past the end of tokens.");
    }
}

