//
// Created by Amit Chavan on 9/2/25.
//

#pragma once

#include "config.h"
#include <cstddef>

/**
 * @enum PageType
 * @brief Types of pages we support.
 */
enum class PageType {
  Header,
  IAM,
  GAM,
  Data,
  Index
};

#pragma pack(1)

/**
 * @struct DatabaseHeader
 * @brief Page 0 for the database. This page contains information to locate GAM, System catalog IAM page,
 *  and other meta data about the database. This is the entry point to how database locates data stored in it.
 */
struct DatabaseHeader {
  const char signature[8] = "MINIDB";
  uint32_t version = 1;
  uint32_t page_size = PAGE_SIZE; // 4096 bytes.
  // This value gets updated as pages get allocated
  uint64_t total_pages = 0;
  // The GAM page is always the 2nd page in the file. Value is 1 since page 0 is db header page.
  page_id_t gam_page_id = 1;

  // This is IAM (Index allocation map) page id which acts as system catalog.
  page_id_t iam_page_id = 2;

  // 6 (sig) + 4 (ver) + 4 (psize) + 8 (tpages) + 4 (gam) + 4 (cat_iam) = 30 bytes
  uint8_t padding[PAGE_SIZE - 30]; // Padding to ensure the header completely fills the page.
};

/**
 * @struct BitmapPage
 * @brief A generic structure that can be used to represent either GAM or IAM page.
 */
struct BitmapPage {
  // Identify the type of page. E.g IAM/GAM
  PageType page_type;

  // When the db grows large we might need to create a chain of GAM or IAM pages.
  page_id_t next_bitmap_page_id = INVALID_PAGE_ID;

  // 4 (type) + 4 (next_id) = 8 bytes for the header
  char bitmap[PAGE_SIZE - 8];
};
#pragma pack()

/**
 * @class Bitmap
 * @brief A helper class to manipulate raw bits stored in the bitmap array insided the BitmapPage class
 *
 */
class Bitmap {
 public:
  /**
   * @brief Constructs a Bitmap wrapper around raw bitmap data.
   * @param data A pointer to the start of the bitmap data (e.g., BitmapPage::bitmap).
   * @param size_in_bits The total number of bits the bitmap can hold.
   */
  explicit Bitmap(char* data, size_t size_in_bits)
	  : data(reinterpret_cast<uint8_t*>(data)), size(size_in_bits) {}

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

 private:
  uint8_t* data;
  size_t size;
};