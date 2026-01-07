#include "iam_manager.h"
#include "common/logger.h"
#include <cstring>
#include <new>
#include <algorithm>

namespace letty {

IamManager::IamManager(DiskManager &disk_manager, ExtentManager &extent_manager)
    : disk_manager_(disk_manager), extent_manager_(extent_manager) {}

page_id_t IamManager::create_iam_chain() {
  // 1. Allocate a raw extent to hold the IAM page (and 7 wasted pages)
  page_id_t new_extent_page = extent_manager_.allocate_extent();
  if (new_extent_page == INVALID_PAGE_ID) {
    return INVALID_PAGE_ID;
  }

  // 2. Initialize the first page as a SparseIamPage covering range 0
  char buffer[PAGE_SIZE] = {0};
  auto iam_page = new(buffer) SparseIamPage();
  iam_page->next_bitmap_page_id = INVALID_PAGE_ID;
  iam_page->extent_range_start = 0; // First IAM page always covers range 0

  // 3. Write to disk
  if (disk_manager_.write_page(new_extent_page, buffer) != IOResult::SUCCESS) {
    return INVALID_PAGE_ID;
  }

  return new_extent_page;
}

page_id_t IamManager::allocate_extent(page_id_t iam_head_page_id) {
  // All IAM operations now use the unified sparse implementation
  return allocate_extent_sparse(iam_head_page_id);
}

page_id_t IamManager::allocate_extent_sparse(page_id_t iam_head_page_id) {
  LOG_STORAGE_DEBUG("Starting sparse extent allocation for IAM chain {}", iam_head_page_id);
  
  // 0. Validate input parameters
  if (iam_head_page_id == INVALID_PAGE_ID) {
    LOG_STORAGE_ERROR("Invalid IAM head page ID provided");
    return INVALID_PAGE_ID;
  }
  
  // 1. Allocate a physical extent from the global pool
  page_id_t extent_start_page = extent_manager_.allocate_extent();
  if (extent_start_page == INVALID_PAGE_ID) {
    LOG_STORAGE_ERROR("Failed to allocate physical extent from ExtentManager");
    return INVALID_PAGE_ID;
  }
  
  // 2. Calculate the global extent index for this extent
  uint64_t global_extent_index = static_cast<uint64_t>(extent_start_page) / EXTENT_SIZE;
  LOG_STORAGE_DEBUG("Allocated physical extent starting at page {}, global index {}", 
                   extent_start_page, global_extent_index);
  
  // 3. Find or create the appropriate IAM page for this extent
  auto [iam_page_id, bit_offset] = find_or_create_iam_page_for_extent(
      iam_head_page_id, global_extent_index);
  
  if (iam_page_id == INVALID_PAGE_ID) {
    LOG_STORAGE_ERROR("Failed to find or create IAM page for extent {}", global_extent_index);
    // TODO: Should ideally deallocate the extent from ExtentManager here
    return INVALID_PAGE_ID;
  }
  
  // 4. Mark the extent as allocated in the IAM page
  char buffer[PAGE_SIZE];
  if (disk_manager_.read_page(iam_page_id, buffer) != IOResult::SUCCESS) {
    LOG_STORAGE_ERROR("Failed to read IAM page {} from disk", iam_page_id);
    return INVALID_PAGE_ID;
  }
  
  auto sparse_iam_page = reinterpret_cast<SparseIamPage*>(buffer);
  Bitmap bitmap(sparse_iam_page->bitmap, SPARSE_MAX_BITS);
  
  // Verify the bit isn't already set (should never happen with correct ExtentManager)
  if (bitmap.is_set(bit_offset)) {
    LOG_STORAGE_ERROR("Extent {} already marked as allocated in IAM - data corruption?", 
                     global_extent_index);
    return INVALID_PAGE_ID;
  }
  
  // Set the bit and write back to disk
  bitmap.set(bit_offset);
  if (disk_manager_.write_page(iam_page_id, buffer) != IOResult::SUCCESS) {
    LOG_STORAGE_ERROR("Failed to write updated IAM page {} to disk", iam_page_id);
    return INVALID_PAGE_ID;
  }
  
  LOG_STORAGE_INFO("Successfully allocated extent {} (page {}) for table", 
                  global_extent_index, extent_start_page);
  return extent_start_page;
}

page_id_t IamManager::create_sparse_iam_page(uint64_t extent_range_start) {
  LOG_STORAGE_DEBUG("Creating sparse IAM page for extent range starting at {}", extent_range_start);
  
  // 1. Allocate a physical extent to hold the new IAM page
  page_id_t new_extent_page = extent_manager_.allocate_extent();
  if (new_extent_page == INVALID_PAGE_ID) {
    LOG_STORAGE_ERROR("Failed to allocate extent for new sparse IAM page");
    return INVALID_PAGE_ID;
  }
  
  // 2. Initialize the page as a SparseIamPage
  char buffer[PAGE_SIZE] = {0};
  auto sparse_iam_page = new(buffer) SparseIamPage();
  sparse_iam_page->next_bitmap_page_id = INVALID_PAGE_ID;
  sparse_iam_page->extent_range_start = extent_range_start;
  
  // 3. Write the initialized page to disk
  if (disk_manager_.write_page(new_extent_page, buffer) != IOResult::SUCCESS) {
    LOG_STORAGE_ERROR("Failed to write new sparse IAM page {} to disk", new_extent_page);
    return INVALID_PAGE_ID;
  }
  
  LOG_STORAGE_INFO("Created sparse IAM page {} covering extent range {}-{}", 
                  new_extent_page, extent_range_start, 
                  extent_range_start + SPARSE_MAX_BITS - 1);
  return new_extent_page;
}

std::tuple<page_id_t, size_t> IamManager::find_or_create_iam_page_for_extent(
    page_id_t iam_head_page_id, uint64_t target_extent_index) {
  
  LOG_STORAGE_DEBUG("Finding IAM page for extent index {}", target_extent_index);
  
  // Calculate which range this extent should be in
  uint64_t target_range_start = calculate_sparse_range_start(target_extent_index);
  
  page_id_t current_page_id = iam_head_page_id;
  page_id_t prev_page_id = INVALID_PAGE_ID;
  
  while (current_page_id != INVALID_PAGE_ID) {
    char buffer[PAGE_SIZE];
    if (disk_manager_.read_page(current_page_id, buffer) != IOResult::SUCCESS) {
      LOG_STORAGE_ERROR("Failed to read IAM page {} during traversal", current_page_id);
      return {INVALID_PAGE_ID, 0};
    }
    
    // All IAM pages are now SparseIamPage
    auto sparse_page = reinterpret_cast<SparseIamPage*>(buffer);
    uint64_t current_range_start = sparse_page->extent_range_start;
    
    LOG_STORAGE_DEBUG("Checking IAM page {} with range start {}", 
                     current_page_id, current_range_start);
    
    // Case 1: Found the exact range we need
    if (current_range_start == target_range_start) {
      size_t bit_offset = target_extent_index - target_range_start;
      LOG_STORAGE_DEBUG("Found existing IAM page {} for extent {}, bit offset {}", 
                       current_page_id, target_extent_index, bit_offset);
      return {current_page_id, bit_offset};
    }
    
    // Case 2: Current range is beyond our target - need to insert before this page
    if (current_range_start > target_range_start) {
      LOG_STORAGE_DEBUG("Need to insert new IAM page before page {} (range {})", 
                       current_page_id, current_range_start);
      page_id_t new_page_id = create_sparse_iam_page(target_range_start);
      if (new_page_id == INVALID_PAGE_ID) {
        return {INVALID_PAGE_ID, 0};
      }
      
      // Link the new page into the chain
      char new_buffer[PAGE_SIZE];
      if (disk_manager_.read_page(new_page_id, new_buffer) != IOResult::SUCCESS) {
        LOG_STORAGE_ERROR("Failed to read newly created IAM page {}", new_page_id);
        return {INVALID_PAGE_ID, 0};
      }
      
      auto new_sparse_page = reinterpret_cast<SparseIamPage*>(new_buffer);
      new_sparse_page->next_bitmap_page_id = current_page_id;
      disk_manager_.write_page(new_page_id, new_buffer);
      
      // Update the previous page to point to the new page
      if (prev_page_id != INVALID_PAGE_ID) {
        char prev_buffer[PAGE_SIZE];
        disk_manager_.read_page(prev_page_id, prev_buffer);
        auto prev_page = reinterpret_cast<SparseIamPage*>(prev_buffer);
        prev_page->next_bitmap_page_id = new_page_id;
        disk_manager_.write_page(prev_page_id, prev_buffer);
      }
      
      size_t bit_offset = target_extent_index - target_range_start;
      LOG_STORAGE_INFO("Inserted new sparse IAM page {} for extent range {}", 
                      new_page_id, target_range_start);
      return {new_page_id, bit_offset};
    }
    
    // Case 3: Continue to next page
    prev_page_id = current_page_id;
    current_page_id = sparse_page->next_bitmap_page_id;
  }
  
  // Case 4: Reached end of chain - append new page
  LOG_STORAGE_DEBUG("Reached end of IAM chain, appending new page for range {}", target_range_start);
  page_id_t new_page_id = create_sparse_iam_page(target_range_start);
  if (new_page_id == INVALID_PAGE_ID) {
    return {INVALID_PAGE_ID, 0};
  }
  
  // Link the new page to the end of the chain
  if (prev_page_id != INVALID_PAGE_ID) {
    char prev_buffer[PAGE_SIZE];
    if (disk_manager_.read_page(prev_page_id, prev_buffer) == IOResult::SUCCESS) {
      auto prev_page = reinterpret_cast<SparseIamPage*>(prev_buffer);
      prev_page->next_bitmap_page_id = new_page_id;
      disk_manager_.write_page(prev_page_id, prev_buffer);
    }
  }
  
  size_t bit_offset = target_extent_index - target_range_start;
  LOG_STORAGE_INFO("Appended new sparse IAM page {} for extent range {}", 
                  new_page_id, target_range_start);
  return {new_page_id, bit_offset};
}

} // namespace letty
