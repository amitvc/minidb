//
// Created by Amit Chavan on 8/30/25.
//

#pragma once

#include "config.h"
#include <fstream>
#include <string>
#include "error_codes.h"

namespace letty {

/**
 * @class DiskManager
 * @brief
 * Component responsible for persisting database pages
 * to stable storage and retrieving them on demand
 * The DiskManager operates strictly at the *page* granularity. A page is
 * the smallest unit of I/O and has a fixed size defined by PAGE_SIZE
 * (see config.h)
 *
 * Responsibilities:
 *  - Read a full page from disk given a page_id
 *  - Write a full page to disk given a page_id
 *  - Allocate new pages by extending the database file
 *  - Maintain durability by ensuring page writes reach stable storage
 *
 * Non-responsibilities:
 *  - Does NOT cache pages (handled by the Buffer Pool)
 *  - Does NOT understand page contents or page types
 *  - Does NOT manage free space within pages
 *  - Does NOT perform concurrency control or locking
 *
 * Page addressing model:
 *  - page_id is a logical identifier
 *  - Physical file offset = page_id * PAGE_SIZE
 *  - Page 0 is always the Database Header Page
 */
class DiskManager {
 public:
  explicit DiskManager(std::string db_file_name);

  /**
   * @brief Shuts down the disk manager, closing the file stream.
   */
  ~DiskManager();

  /**
   * @brief Writes the contents of the page to the db.
   * @param page_id  ID of the page that is being written.
   * @param data  Data that is to be written.
   */
  IOResult write_page(page_id_t page_id, const char *data);

  /**
   * @brief Reads the content of specified page in the accompanying buffer
   * @param page_id ID of the page that is being written.
   * @param buffer Buffer in which data will be written.
   */
  IOResult read_page(page_id_t page_id, char *buffer);

 private:
  std::string file_name_;
  std::fstream db_file_;
};
}