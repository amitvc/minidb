//
// Created by Amit Chavan on 9/15/25.
//

#pragma once

#include "disk_manager.h"
#include <mutex>

namespace letty {
/**
 * @class ExtentManager
 * @brief Manages allocation and deallocation of Extent's within the database file.
 * An *extent* is a contiguous group of pages treated as a single allocation unit. All pages within an Extent typically
 * belong to a single database entity (either table, index or system catalog). For letty the size of extent is 8 pages
 * (see EXTENT_SIZE in config.h)
 * The ExtentManager sits above the DiskManager and below higher-level components.
 *
 * Thread-safety:
 *  - ExtentManager is thread-safe.
 *  - All public methods are internally synchronized.
 *  - A single mutex protects all extent allocation metadata
 *    (GAM pages, IAM pages, and related in-memory state).
 *
 * Locking model:
 *  - Coarse-grained locking is used to keep allocation logic simple
 *    and correct.
 *  - Only one extent allocation or deallocation may proceed at a time.
 *
 * Responsibilities:
 *  - Allocate new extents.
 *  - Free extents when objects are dropped or truncated.
 *  - Track global extent usage via the Global Allocation Map (GAM)
 *  - Track per-object extent ownership via Index Allocation Maps (IAMs)
 *
 * Reference:
 *  - GAM / IAM allocation strategy inspired by SQL Server internals
 *    https://tinyurl.com/32rhava7
 */
class ExtentManager {
 public:
  explicit ExtentManager(DiskManager &disk_manager);

  /**
   * @brief Allocates a new extent
   * @return PageId of the first page in the newly allocated extent.
   */
  page_id_t allocate_extent();

  /**
   * @brief Reclaims the extent
   * @param start_page_id The starting pageId of the extent that is being deallocated.
   */
  void deallocate_extent(page_id_t start_page_id);
 private:
  /**
   * @brief Initializes a brand new database file.
   * This function is called by the constructor if it detects that the database
   * file is empty. It creates and writes the initial Header and GAM pages.
   */
  void initialize_new_db();

  DiskManager &disk_manager_;
  //
  std::mutex lock_;
  // We keep track of the last know free GAM page so we don't need to scan each gam page to arrive at the free
  // gam page. This essentially removes the O(n) linear scan of all gam pages.
  page_id_t last_known_free_gam_id_ = FIRST_GAM_PAGE_ID;
  // The last known free index in the last know free gam page.
  size_t last_known_free_gam_index_ = 0;

  // Caching the active GAM page to avoid frequent reads.
  // We use a raw char array here, but in a real system this might be a BufferPool page.
  alignas(4096) char gam_page_cache_[PAGE_SIZE];
  page_id_t cached_gam_page_id_ = INVALID_PAGE_ID;
};
}
