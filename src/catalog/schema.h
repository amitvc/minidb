#pragma once

#include <vector>
#include <optional>
#include "catalog/column.h"

namespace letty {

/**
 * @class Schema
 * @brief Represents the schema of a table (a collection of columns).
 */
class Schema {
 public:
  Schema() : length_(0) {}
  explicit Schema(const std::vector<Column> &columns) : columns_(columns) {
	// Calculate tuple length
	length_ = 0;
	for (const auto &col : columns) {
	  length_ += col.get_length();
	}
  }

  const std::vector<Column> &get_columns() const { return columns_; }

  const Column *get_column(const std::string &name) const {
	for (const auto &col : columns_) {
	  if (col.get_name() == name) {
		return &col;
	  }
	}
	return nullptr;
  }

  uint32_t get_length() const { return length_; }

 private:
  std::vector<Column> columns_;
  uint32_t length_;
};
}
