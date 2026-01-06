//
// Created by Amit Chavan on 9/2/25.
//

#pragma once

#include "config.h"
#include "sql/utils.h"
#include <cstddef>
#include <cstring>


namespace minidb {
/**
 * @enum PageType
 * @brief Types of pages we support.
 */
enum class PageType {
  Header,
  IAM,
  GAM,
  Catalog,
  Data,
  Index,
  // Catalog - Removed, sys tables are just Data pages
};

#pragma pack(1) // turn off byte padding

/**
 * @struct DatabaseHeader
 * @brief Page 0 for the database. This page contains information to locate GAM, System catalog IAM page,
 *  and other meta data about the database. This is the entry point to how database locates data stored on disk.
 *
 *  * Database Header Page (Page 0)
 *
 * This page is always the FIRST page in the database file and defines
 * the global metadata for the entire database. It has a fixed layout
 * and always occupies exactly one page (PAGE_SIZE bytes).
 *
 * Byte Offsets (PAGE_SIZE = 4096)
 *
 *  +--------------------------------------------------------------+ 0
 *  | signature[8]   ("MINIDB\0\0")                                |
 *  |   - Identifies this file as a MiniDB database                |
 *  +--------------------------------------------------------------+ 8
 *  | version (uint32)                                             |
 *  |   - Database file format version                             |
 *  +--------------------------------------------------------------+ 12
 *  | page_size (uint32)                                           |
 *  |   - Size of each page in bytes (e.g., 4096)                  |
 *  +--------------------------------------------------------------+ 16
 *  | total_pages (uint64)                                         |
 *  |   - Total number of pages allocated in the database file     |
 *  +--------------------------------------------------------------+ 24
 *  | gam_page_id (page_id_t)                                      |
 *  |   - Page ID of the Global Allocation Map (GAM)               |
 *  |   - Always page 1                                            |
 *  +--------------------------------------------------------------+ 28
 *  | sys_tables_iam_page (page_id_t)                              |
 *  |   - IAM page for system catalog: sys_tables                  |
 *  +--------------------------------------------------------------+ 32
 *  | sys_columns_iam_page (page_id_t)                             |
 *  |   - IAM page for system catalog: sys_columns                 |
 *  +--------------------------------------------------------------+ 36
 *  | padding                                                      |
 *  |   - Zero-filled                                              |
 *  |   - Reserved for future metadata                             |
 *  |   - Ensures the header occupies exactly one full page        |
 *  +--------------------------------------------------------------+ PAGE_SIZE
 *
 * Invariants:
 *  - This page is always page_id = 0
 *  - Layout is fixed and backward-compatible via `version`
 *  - All other pages rely on this page for bootstrapping
 */
struct DatabaseHeader {
  char signature[8];
  uint32_t version = 1;
  uint32_t page_size = PAGE_SIZE; // 4096 bytes.
  // This value gets updated as pages get allocated
  uint64_t total_pages = 0;
  // The GAM page is always the 2nd page in the file. Value is 1 since page 0 is db header page.
  page_id_t gam_page_id = 1;

  // Points to the IAM page for the 'sys_tables' table (The Catalog of Tables)
  page_id_t sys_tables_iam_page = 2;

  // Points to the IAM page for the 'sys_columns' table (The Catalog of Columns)
  page_id_t sys_columns_iam_page = 3;

  // 8 (sig) + 4 (ver) + 4 (psize) + 8 (tpages) + 4 (gam) + 4 (sys_tables) + 4 (sys_cols) = 36 bytes
  uint8_t padding[PAGE_SIZE - 36]; // Padding to ensure the header completely fills the page.

  DatabaseHeader() {
	  std::memset(signature, 0, sizeof(signature));  // zero pad the entire field
	  std::memcpy(signature, DB_SIGNATURE, sizeof(DB_SIGNATURE));
  }
};

/**
 * @struct BitmapPage
 * @brief A generic structure to represent Allocation Map pages (GAM or IAM).
 *
 * This struct overlays a raw 4KB page. It consists of:
 * 1. A small header (page type and pointer to the next bitmap page).
 * 2. A large byte array (`bitmap`) effectively holding 32704 bits.
 *
 * - usage as GAM: Each bit represents an Extent (8 pages). 1 GAM page can track 32704 extents (~1GB of data).
 * - usage as IAM: Each bit represents an Extent belonging to a specific table/index.
 * Note: Each extent represents a group of 8 consecutive 4kb pages.
 *
 *  +--------------------------------------------------------------+ 0
 *  | page_type (PageType)                                         |
 *  |   - Identifies the meaning of bits (GAM vs IAM)              |
 *  +--------------------------------------------------------------+ 4
 *  | next_bitmap_page_id (page_id_t)                              |
 *  |   - Chains bitmap pages if we need to track more extents     |
 *  +--------------------------------------------------------------+ 8
 *  | bitmap[BITMAP_ARRAY_SIZE]                                    |
 *  |   - Raw bitset payload                                       |
 *  |   - BITMAP_ARRAY_SIZE = PAGE_SIZE - 4 - 4 = 4088 bytes       |
 *  |   - bits_per_bitmap_page = 4088 * 8 = 32704 extents          |
 *  |   - pages_covered = 32704 extents * 8 pages/extent           |
 *  |                  = 261,632 pages                             |
 *  +--------------------------------------------------------------+
 *
 */
struct BitmapPage {
  // Identify the type of page. E.g IAM/GAM
  PageType page_type;

  // When the db grows large we might need to create a chain of GAM or IAM pages.
  page_id_t next_bitmap_page_id = INVALID_PAGE_ID;

  // PageSize  - 4 (type) - 4 (next_id)
  char bitmap[BITMAP_ARRAY_SIZE]; // 4088 bytes
};
#pragma pack()

/**
 * @class Bitmap
 * @brief A helper class to manipulate raw bits stored in the bitmap array inside the BitmapPage class.
 *
 * This class provides an abstraction over raw byte arrays to allow for easy setting, clearing,
 * and checking of individual bits. This is primarily used for managing Allocation Maps (GAM/IAM),
 * where each bit represents the status (allocated/free) of an extent (for GAM) or page (for IAM).
 */
class Bitmap {
 public:
  /**
   * @brief Constructs a Bitmap wrapper around raw bitmap data.
   * @param data A pointer to the start of the bitmap data (e.g., BitmapPage::bitmap).
   * @param size_in_bits The total number of bits the bitmap can hold.
   */
  explicit Bitmap(char *data, size_t size_in_bits)
	  : data(reinterpret_cast<uint8_t *>(data)), size(size_in_bits) {}

  /**
   * @brief Checks if a specific bit is set to 1.
   * @param bit_index The index of the bit to check.
   * @return True if the bit is set, false otherwise.
   */
  bool is_set(uint32_t bit_index) const {
	if (bit_index >= size) return false;
	// Find the byte, then check the specific bit within that byte.
	return (data[bit_index / 8] & (1 << (bit_index % 8))) != 0;
  }

  /**
   * @brief Sets a specific bit to 1.
   * @param bit_index The index of the bit to set.
   */
  void set(uint32_t bit_index) {
	if (bit_index >= size) return;
	data[bit_index / 8] |= (1 << (bit_index % 8));
  }

  /**
   * @brief Clears a specific bit, setting it to 0.
   * @param bit_index The index of the bit to clear.
   */
  void clear(uint32_t bit_index) {
	if (bit_index >= size) return;
	data[bit_index / 8] &= ~(1 << (bit_index % 8));
  }

  /**
   * @brief Returns the size of the bitmap.
   * @return
   */
  size_t get_size_in_bits() {
	return size;
  }
 private:
  uint8_t *data;
  size_t size;
};
}