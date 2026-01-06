//
// Created by Amit Chavan on 12/26/25.
//

#pragma once

#include "disk_manager.h"
#include "storage_def.h"
#include "extent_manager.h"

namespace minidb {

/**
 * @class IamManager
 * @brief Responsible for managing Extents for a table/Index.
 *
 * GAM tracks all extents in the system whereas IAM (Index Allocation Map) tracks which extents belong to a
 * specific object (like a Table)
 */
class IamManager {
 public:
  IamManager(DiskManager &disk_manager, ExtentManager &extent_manager);

  /**
   * @brief Creates a new IAM chain.
   * Allocates a new extent, initializes the first page as an IAM page, and returns its Page ID.
   * Used when creating a new table.
   * @return The Page ID of the new IAM page.
   */
  page_id_t create_iam_chain();

  /**
   * @brief Allocates an extent for a specific IAM chain.
   * Searches the IAM page(s) for a free slot to claim an extent.
   * If successful, it asks ExtentManager for a physical extent and links it here.
   *
   * @param iam_head_page_id The start of the IAM chain for the table.
   * @return The starting Page ID of the newly allocated extent, or INVALID_PAGE_ID if failed.
   */
  page_id_t allocate_extent(page_id_t iam_head_page_id);
  
 private:
  DiskManager &disk_manager_;
  ExtentManager &extent_manager_;
};

} // namespace minidb
