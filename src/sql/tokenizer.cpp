//
// Created by Amit Chavan on 6/9/25.
//

#include "tokenizer.h"
#include <sstream>
#include <unordered_set>

namespace minidb {

    static const std::unordered_set<std::string> keywords = {
            // DDL
            "create", "table", "drop",

            // DML
            "insert", "into", "values",
            "select", "from", "where",
            "update", "set",
            "delete",

            // expressions
            "and", "or", "not", "in", "like",
            "is", "null", "as", "distinct",

            // ordering & limits
            "order", "by", "asc", "desc",
            "limit", "offset"
    };



}
