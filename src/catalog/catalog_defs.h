#pragma once

#include <cstdint>
#include "storage/storage_def.h"

namespace minidb {

/**
 * @brief OIDs for system tables
 */
constexpr uint32_t SYS_TABLES_TABLE_OID = 1;
constexpr uint32_t SYS_COLUMNS_TABLE_OID = 2;

// This is hard limit we have on table names, column names.
constexpr size_t MAX_NAME_LENGTH = 32;

enum class DataType : uint8_t {
  INTEGER = 0,
  DOUBLE  = 1,
  VARCHAR = 2,
  BOOLEAN = 3,
  DATE = 4,
  TIMESTAMP = 5
};

/**
 * @struct SysTablesRecord
 * @brief Physical layout of a tuple in the 'sys_tables' table.
 */
struct SysTablesRecord {
  uint32_t oid;
  char name[MAX_NAME_LENGTH];
  page_id_t first_page_id;
  uint16_t column_count;
};

/**
 * @struct SysColumnsRecord
 * @brief Physical layout of a tuple in the 'sys_columns' table.
 */
struct SysColumnsRecord {
  uint32_t table_oid;
  char name[MAX_NAME_LENGTH];
  DataType type;
  uint16_t length;
  uint16_t offset;
};

} // namespace minidb
