//
// Created by Amit Chavan on 12/26/25.
//

#pragma once

#include "disk_manager.h"
#include "storage_def.h"
#include "extent_manager.h"
#include <tuple>

namespace letty {

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
  
  /**
   * @brief Allocates an extent using optimized sparse IAM chain.
   * 
   * This method implements sparse IAM allocation that avoids creating empty
   * intermediate IAM pages. When a table needs an extent in a far range
   * (e.g., extent #200000), it creates only the necessary IAM page for that
   * range instead of creating all intermediate pages.
   *
   * @param iam_head_page_id The start of the IAM chain for the table.
   * @return The starting Page ID of the newly allocated extent, or INVALID_PAGE_ID if failed.
   */
  page_id_t allocate_extent_sparse(page_id_t iam_head_page_id);
  
  /**
   * @brief Creates a sparse IAM page for a specific extent range.
   * 
   * @param extent_range_start The starting global extent index this page should cover
   * @return The Page ID of the new sparse IAM page, or INVALID_PAGE_ID if failed.
   */
  page_id_t create_sparse_iam_page(uint64_t extent_range_start);
  
  /**
   * @brief Finds or creates the appropriate IAM page for a given global extent index.
   * 
   * Traverses the sparse IAM chain to find a page that covers the target extent.
   * If no such page exists, creates one and inserts it in the correct position
   * to maintain sorted order by extent range.
   * 
   * @param iam_head_page_id The start of the IAM chain
   * @param target_extent_index The global extent index to find/create coverage for
   * @return Tuple of (iam_page_id, bit_offset) or (INVALID_PAGE_ID, 0) if failed
   */
  std::tuple<page_id_t, size_t> find_or_create_iam_page_for_extent(
      page_id_t iam_head_page_id, uint64_t target_extent_index);
  
  /**
   * @brief Calculate which sparse IAM range should contain the given extent.
   * Public for testing purposes.
   * 
   * @param global_extent_index The target global extent index
   * @return The starting extent index for the range that should contain this extent
   */
  uint64_t calculate_sparse_range_start(uint64_t global_extent_index) const {
    return (global_extent_index / SPARSE_MAX_BITS) * SPARSE_MAX_BITS;
  }
  
 private:
  DiskManager &disk_manager_;
  ExtentManager &extent_manager_;
};

} // namespace letty
