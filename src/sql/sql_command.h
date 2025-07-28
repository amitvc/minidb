//
// Created by Amit Chavan on 7/6/25.
//

#pragma once

#include <string>
namespace minidb {

    // Supported data types
    enum class DataType { BOOL, INT, FLOAT, VARCHAR };

    enum class Operator { EQ, GT, LT, GE, LE, NE, AND, OR };

    struct ColumnDef {
        std::string name;
        DataType type;
        int length = 0;  // For VARCHAR
    };

    struct Value {
        std::variant<int, float, std::string, bool> data;
    };

    struct Condition {
        enum class NodeType { LEAF, AND, OR };
        std::string column;
        Operator op;
        Value value;
        NodeType type;

        // For AND/OR nodes
        std::unique_ptr<Condition> left;
        std::unique_ptr<Condition> right;

        static std::unique_ptr<Condition> make_leaf(std::string col, Operator o, Value val) {
            auto cond = std::make_unique<Condition>();
            cond->type = NodeType::LEAF;
            cond->column = std::move(col);
            cond->op = o;
            cond->value = std::move(val);
            return cond;
        }

        static std::unique_ptr<Condition> make_and(std::unique_ptr<Condition> l, std::unique_ptr<Condition> r) {
            auto cond = std::make_unique<Condition>();
            cond->type = NodeType::AND;
            cond->left = std::move(l);
            cond->right = std::move(r);
            return cond;
        }

        static std::unique_ptr<Condition> make_or(std::unique_ptr<Condition> l, std::unique_ptr<Condition> r) {
            auto cond = std::make_unique<Condition>();
            cond->type = NodeType::OR;
            cond->left = std::move(l);
            cond->right = std::move(r);
            return cond;
        }
    };

    class ASTNode {
    public:
        virtual ~ASTNode() = default;
    };

    class CreateTableNode : public ASTNode {
        public:
            std::string table_name;
            std::vector<ColumnDef> columns;
    };

    class InsertNode : public ASTNode {
    public:
        std::string table_name;
        std::vector<std::string> columns;
        std::vector<Value> values;
    };

    class SelectNode : public ASTNode {
    public:
        bool select_all = false;
        std::vector<std::string> columns;
        std::string table_name;
        std::vector<Condition> conditions;  // Simplified: AND chain
    };

    class UpdateNode : public ASTNode {
    public:
        std::string table_name;
        std::vector<std::pair<std::string, Value>> assignments;
        std::vector<Condition> conditions;
    };

    class DeleteNode : public ASTNode {
    public:
        std::string table_name;
        std::vector<Condition> conditions;
    };


}