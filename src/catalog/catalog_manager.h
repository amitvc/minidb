#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <unordered_map>

#include "catalog/catalog_defs.h"
#include "catalog/schema.h"
#include "storage/disk_manager.h"
#include "storage/iam_manager.h"

namespace minidb {

/**
 * @struct TableMetadata
 * @brief In-memory representation of a table's metadata.
 * This structure describes the logical and physical properties of a table
 * known to the database catalog. It does NOT store any table data (tuples);
 * it only describes how and where that data is stored.
 */
struct TableMetadata {
  uint32_t oid;
  std::string name; // table name.
  Schema schema;

  //Page identifier of the first page in the table's heap storage.
  //Used by the storage layer to locate and scan the table.
  page_id_t first_page_id;
};

/**
 * @class CatalogManager
 * @brief The CatalogManager is the authoritative owner of database metadata.
 * It manages and serves information about:
 * - Stores Table metadata when a table is created
 * - Given table name returns Table metadata
 * - TODO
 */
class CatalogManager {
 public:
  CatalogManager(DiskManager& disk_manager, IamManager& iam_manager);

  /**
   * @brief Initializes the catalog manager.
   * Checks if sys_tables and sys_columns exist. If not, creates them (Bootstraps).
   */
  void init();

  /**
   * @brief Creates a new table.
   * @param name The name of the table.
   * @param schema The schema of the table.
   * @return The OID of the newly created table, or error if failed.
   */
  bool create_table(const std::string& name, const Schema& schema);

  /**
   * @brief Retrieves metadata for a table by name.
   * @param name The name of the table.
   * @return Pointer to TableMetadata if found, nullptr otherwise.
   */
  std::unique_ptr<TableMetadata> get_table(const std::string& name);

 private:
  DiskManager& disk_manager_;
  IamManager& iam_manager_;

  // Function to create the system tables if they don't exist
  void bootstrap();
  
  // Helper to insert a row into a table (by OID/PageID)
  // For now, this might just handle the specific bootstrapping logic manually
  void insert_tuple(page_id_t first_page_id, const char* data, uint32_t size);
};

}
