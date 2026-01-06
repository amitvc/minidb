#include "iam_manager.h"
#include <cstring>
#include <new>

namespace minidb {

IamManager::IamManager(DiskManager &disk_manager, ExtentManager &extent_manager)
    : disk_manager_(disk_manager), extent_manager_(extent_manager) {}

page_id_t IamManager::create_iam_chain() {
  // 1. Allocate a raw extent to hold the IAM page (and 7 wasted pages)
  page_id_t new_extent_page = extent_manager_.allocate_extent();
  if (new_extent_page == INVALID_PAGE_ID) {
    return INVALID_PAGE_ID;
  }

  // 2. Initialize the first page as an IAM Page
  char buffer[PAGE_SIZE] = {0};
  auto iam_page = new(buffer) BitmapPage();
  iam_page->page_type = PageType::IAM;
  iam_page->next_bitmap_page_id = INVALID_PAGE_ID;

  // 3. Write to disk
  if (disk_manager_.write_page(new_extent_page, buffer) != IOResult::SUCCESS) {
    return INVALID_PAGE_ID;
  }

  return new_extent_page;
}

page_id_t IamManager::allocate_extent(page_id_t iam_head_page_id) {
  // 1. Allocate a real physical extent
  page_id_t extent_start_page = extent_manager_.allocate_extent();
  if (extent_start_page == INVALID_PAGE_ID) {
    return INVALID_PAGE_ID;
  }

  // 2. Calculate which IAM Page SHOULD hold this bit.
  // Each IAM page holds bit for 'MAX_BITS' extents.
  // Global Extent Index = start_page / 8
  // IAM Page Index = Global Extent Index / MAX_BITS
  size_t global_extent_index = extent_start_page / EXTENT_SIZE;
  size_t bits_per_iam_page = MAX_BITS;
  size_t target_iam_index = global_extent_index / bits_per_iam_page;
  size_t bit_offset_in_page = global_extent_index % bits_per_iam_page;

  // 3. Traverse the IAM chain to find (or create) the target IAM page
  page_id_t current_iam_page_id = iam_head_page_id;
  size_t current_index = 0;

  while (current_index < target_iam_index) {
    char buffer[PAGE_SIZE];
    if (disk_manager_.read_page(current_iam_page_id, buffer) != IOResult::SUCCESS) {
      // Very bad error: Valid chain broken locally?
      return INVALID_PAGE_ID;
    }
    auto iam_page = reinterpret_cast<BitmapPage*>(buffer);

    if (iam_page->next_bitmap_page_id == INVALID_PAGE_ID) {
      // We reached end of chain, but we need to go further.
      // Create a NEW IAM page and link it.
      page_id_t new_iam_page = create_iam_chain();
      if (new_iam_page == INVALID_PAGE_ID) {
        return INVALID_PAGE_ID; // Failed to extend chain
      }
      
      // Link current -> new
      iam_page->next_bitmap_page_id = new_iam_page;
      disk_manager_.write_page(current_iam_page_id, buffer);
    }
    
    // Move forward
    current_iam_page_id = iam_page->next_bitmap_page_id;
    current_index++;
  }

  // 4. We are at the correct IAM page. Mark the bit.
  char buffer[PAGE_SIZE];
  if (disk_manager_.read_page(current_iam_page_id, buffer) != IOResult::SUCCESS) {
    return INVALID_PAGE_ID;
  }
  auto iam_page = reinterpret_cast<BitmapPage*>(buffer);
  
  Bitmap bitmap(iam_page->bitmap, MAX_BITS);
  if (bitmap.is_set(bit_offset_in_page)) {
    // This should theoretically not happen if ExtentManager works correctly,
    // as ExtentManager shouldn't give us an allocated extent.
    // However, for robustness:
    return INVALID_PAGE_ID; 
  }
  
  bitmap.set(bit_offset_in_page);
  disk_manager_.write_page(current_iam_page_id, buffer);

  return extent_start_page;
}

} // namespace minidb
