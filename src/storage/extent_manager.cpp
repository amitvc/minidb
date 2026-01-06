//
// Created by Amit Chavan on 9/15/25.
//

#include "storage_def.h"

#include "extent_manager.h"
#include "common/logger.h"
#include <new>
#include <stdexcept>
#include <string>

namespace minidb {

page_id_t ExtentManager::allocate_extent() {
  LOG_STORAGE_DEBUG("Starting extent allocation");
  std::lock_guard<std::mutex> guard(lock_);
  page_id_t current_gam_page_id = last_known_free_gam_id_;
  size_t gam_page_index = last_known_free_gam_index_;
  LOG_STORAGE_DEBUG("Starting search from GAM page {} at index {}", current_gam_page_id, gam_page_index);

  while (true) {
    // 1. Read the GAM page (Use Cache if possible)
    BitmapPage* gam_page;
    if (current_gam_page_id == cached_gam_page_id_) {
        // Cache Hit!
        LOG_STORAGE_DEBUG("GAM cache hit for page {}", current_gam_page_id);
        gam_page = reinterpret_cast<BitmapPage*>(gam_page_cache_);
    } else {
        // Cache Miss - Read from disk and update cache
        LOG_STORAGE_DEBUG("GAM cache miss for page {}, reading from disk", current_gam_page_id);
        if (disk_manager_.read_page(current_gam_page_id, gam_page_cache_) != IOResult::SUCCESS) {
            LOG_STORAGE_ERROR("Failed to read GAM page {} from disk", current_gam_page_id);
            return INVALID_PAGE_ID;
        }
        cached_gam_page_id_ = current_gam_page_id;
        gam_page = reinterpret_cast<BitmapPage*>(gam_page_cache_);
    }

    Bitmap bitmap(gam_page->bitmap, MAX_BITS);
    size_t bits_per_gam = bitmap.get_size_in_bits();

    for (size_t i = 0; i < bits_per_gam; ++i) {
	  // Find the first free bit
      if (!bitmap.is_set(i)) {
        // Mark it as allocated.
        bitmap.set(i);
        // Write back to disk immediately
        disk_manager_.write_page(current_gam_page_id, gam_page_cache_);
        LOG_STORAGE_DEBUG("Found free bit {} in GAM page {}", i, current_gam_page_id);

        /*
         * The calculation of page id below is important to understand.
         * We use GAM page ID and the bit set within this page to calculate the page id we return to the caller.
         *
         * 1. gam_page_index: Which GAM page are we looking at?
         * 2. bits_per_gam: How many extents does one GAM page cover?
         *    - Each bit represents one EXTENT.
         *    - If PAGE_SIZE=4096, a GAM has ~32,704 bits.
         *    - So one GAM page covers ~32,704 extents.
         *    - 8 bits are reserved for page type and next page id. See the BitmapPage struct in storage_def.h
         *
         * 3. EXTENT_SIZE: How many pages are in one extent? (Defined as 8)
         *
         * FORMULA:
         * Absolute Page ID = (Pages covered by previous GAMs) + (Pages covered by bits before 'i' in current GAM)
         *
         * EXAMPLE:
         * Pattern: [GAM 0] [GAM 1] ...
         * Let's say bits_per_gam = 4 (simplification) and EXTENT_SIZE = 8.
         *
         * Case A: gam_page_index = 0, i = 2 (3rd bit)
         *    base_page_id = 0 * 4 * 8 = 0
         *    offset       = 2 * 8 = 16
         *    Result       = Page 16. (Extent 2 covers pages 16-23)
         *
         * Case B: gam_page_index = 1, i = 2 (3rd bit of 2nd GAM)
         *    base_page_id = 1 * 4 * 8 = 32
         *    offset       = 2 * 8 = 16
         *    Result       = Page 48.
         */
        page_id_t base_page_id = static_cast<page_id_t>(gam_page_index * bits_per_gam * EXTENT_SIZE);
        page_id_t result = base_page_id + static_cast<page_id_t>(i * EXTENT_SIZE);
        
        LOG_STORAGE_INFO("Successfully allocated extent at page {}", result);
        return result;
      }
    }

    // No free space in this GAM page.
    if (gam_page->next_bitmap_page_id != INVALID_PAGE_ID) {
        current_gam_page_id = gam_page->next_bitmap_page_id;
        // Optimization: If this page is full, we never need to check it again *until* a deallocation happens.
        // So we can safely advance our cached start pointer.
        last_known_free_gam_id_ = current_gam_page_id;
        gam_page_index++;
        last_known_free_gam_index_ = gam_page_index;
        continue;
    }

    // No next GAM page. Create one!
    // Determining location for new GAM page.
    // Try to pack in Extent 0 if possible.
    page_id_t new_gam_page_id = INVALID_PAGE_ID;
    bool is_packed = false;

    // Check if we can put it in the reserved Extent 0 (Pages 0-7)
    // Current layout: 0:Header, 1:GAM1, 2:IAM
    // So candidates are 3, 4, 5, 6, 7.
    // We check if current_gam_page_id is < 7. If so, next one might be free.
    // Actually, we should check specifically if we are creating the 2nd, 3rd... GAM.
    // Extent 0 can hold GAMs up to Page 7.

    // Simplistic check: If current is < 7, try current + 1.
    // If current + 1 == 2 (IAM), skip to 3.
    // Note: This relies on the fact that we chain them in order.

    page_id_t candidate = current_gam_page_id + 1;
    if (candidate == 2) candidate += 2; // Skip IAM pages (2 and 3)

    if (candidate < EXTENT_SIZE) {
        // We have space in Extent 0!
        new_gam_page_id = candidate;
        is_packed = true;
    } else {
        // Extent 0 is full. Append to file.
        char header_buffer[PAGE_SIZE];
        if (disk_manager_.read_page(HEADER_PAGE_ID, header_buffer) != IOResult::SUCCESS) return INVALID_PAGE_ID;
        auto header = reinterpret_cast<DatabaseHeader*>(header_buffer);

        new_gam_page_id = header->total_pages;
        header->total_pages += EXTENT_SIZE;
        disk_manager_.write_page(HEADER_PAGE_ID, header_buffer);

        is_packed = false;
    }

    // 3. Initialize the new GAM page
    // We construct it directly in a temp buffer, or we can use the cache if we are careful.
    // Let's use a temp buffer to avoid clobbering the current one before we link it.
    char new_gam_buffer[PAGE_SIZE] = {0};
    auto new_gam_page = new(new_gam_buffer) BitmapPage();
    new_gam_page->page_type = PageType::GAM;
    new_gam_page->next_bitmap_page_id = INVALID_PAGE_ID;

    if (!is_packed) {
        // If we allocated a new extent for this GAM, mark it as used to protect itself.
        Bitmap new_gam_bitmap(new_gam_page->bitmap, MAX_BITS);
        new_gam_bitmap.set(0);
    }

    disk_manager_.write_page(new_gam_page_id, new_gam_buffer);

    // 4. Link the old GAM page to the new one
    // The old GAM page is currently in 'gam_page_cache_' (guaranteed by the loop start logic)
    gam_page->next_bitmap_page_id = new_gam_page_id;
    disk_manager_.write_page(current_gam_page_id, gam_page_cache_);

    // Update iterator
    current_gam_page_id = new_gam_page_id;
    gam_page_index++;
  }
}

void ExtentManager::deallocate_extent(page_id_t start_page_id) {
    LOG_STORAGE_INFO("Deallocating extent at page {}", start_page_id);
    std::lock_guard<std::mutex> guard(lock_);

    size_t extent_index = start_page_id / EXTENT_SIZE;
    size_t bits_per_gam = MAX_BITS;
    size_t gam_page_index = extent_index / bits_per_gam;
    size_t bit_in_gam = extent_index % bits_per_gam;

    // Find the GAM page
    page_id_t current_gam_page_id = FIRST_GAM_PAGE_ID;
    for (size_t i = 0; i < gam_page_index; ++i) {
        char buffer[PAGE_SIZE];
        if (disk_manager_.read_page(current_gam_page_id, buffer) != IOResult::SUCCESS) {
            return; // Should not happen in a consistent DB
        }
        auto gam_page = reinterpret_cast<BitmapPage *>(buffer);
        if (gam_page->next_bitmap_page_id == INVALID_PAGE_ID) {
            return; // DB inconsistency
        }
        current_gam_page_id = gam_page->next_bitmap_page_id;
    }

    // Clear the bit
    // Check if the page we need to modify is already in cache
    char* target_buffer;
    char temp_buffer[PAGE_SIZE];

    if (current_gam_page_id == cached_gam_page_id_) {
        target_buffer = gam_page_cache_;
    } else {
        target_buffer = temp_buffer;
        if (disk_manager_.read_page(current_gam_page_id, target_buffer) != IOResult::SUCCESS) {
            return;
        }
    }

    auto gam_page = reinterpret_cast<BitmapPage *>(target_buffer);
    Bitmap bitmap(gam_page->bitmap, bits_per_gam);
    bitmap.clear(bit_in_gam);

    // Write back to disk
    disk_manager_.write_page(current_gam_page_id, target_buffer);

    // Optimization: if we deallocated in a GAM page before our cached "last known free",
    // we should rewind our cache to ensure we can reuse this newly freed space.
    if (gam_page_index < last_known_free_gam_index_) {
        last_known_free_gam_index_ = gam_page_index;
        last_known_free_gam_id_ = current_gam_page_id;
    }
}

/**
 * @brief Bootstraps a fresh database file with initial system pages.
 * Creates:
 * - Page 0: Database Header
 * - Page 1: GAM (Global Allocation Map)
 * - Page 2: IAM (Index Allocation Map for System Catalog)
 */
void ExtentManager::initialize_new_db() {
  LOG_STORAGE_INFO("Initializing new database file");
  // 1. Prepare Header Page (Page 0)
  char header_buffer[PAGE_SIZE] = {0};
  auto header = new(header_buffer) DatabaseHeader();

  // We allocate the first extent (8 pages) to hold these system pages.
  // Although we only use 3 pages now, we reserve the whole block.
  header->total_pages = EXTENT_SIZE;
  header->gam_page_id = FIRST_GAM_PAGE_ID;

  // 2. Prepare GAM Page (Page 1)
  char gam_buffer[PAGE_SIZE] = {0};
  auto gam_page = new(gam_buffer) BitmapPage();
  gam_page->page_type = PageType::GAM;

  // Mark Extent 0 as allocated (it holds Header, GAM, IAM)
  Bitmap gam_bitmap(gam_page->bitmap, MAX_BITS);
  gam_bitmap.set(0);

  // 3. Prepare IAM Page (Page 2) - This will be our System Catalog
  char iam_buffer[PAGE_SIZE] = {0};
  auto iam_page = new(iam_buffer) BitmapPage();
  iam_page->page_type = PageType::IAM;

  // 4. Write all to disk
  disk_manager_.write_page(HEADER_PAGE_ID, header_buffer);
  disk_manager_.write_page(FIRST_GAM_PAGE_ID, gam_buffer);
  disk_manager_.write_page(SYS_TABLES_IAM_PAGE_ID, iam_buffer); // Page 2 is IAM for sys_tables
  disk_manager_.write_page(SYS_COLUMNS_IAM_PAGE_ID, iam_buffer); // Page 3 is IAM for sys_columns
}

ExtentManager::ExtentManager(DiskManager &disk_manager) : disk_manager_(disk_manager) {
  LOG_STORAGE_INFO("Initializing ExtentManager");
  std::memset(gam_page_cache_, 0, PAGE_SIZE);
  char buffer[PAGE_SIZE];
  if (disk_manager_.read_page(HEADER_PAGE_ID, buffer) != IOResult::SUCCESS) {
      // Read failed (likely empty file), so initialize new DB
      initialize_new_db();
  } else {
      // Read succeeded, verify signature to ensure it's a valid MiniDB file
      auto* header = reinterpret_cast<DatabaseHeader*>(buffer);
      if (std::string(header->signature) != DB_SIGNATURE) {
          throw std::runtime_error("Corrupt or invalid database file: Signature mismatch. Expected 'MINIDB'.");
      }
  }
}

}