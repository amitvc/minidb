//
// Created by Amit Chavan on 8/30/25.
//

#pragma once

#include "config.h"
#include <fstream>
#include <string>
#include "error_codes.h"

/**
 * @class DiskManager
 * @brief Class manages read/writing database pages to the file system.
 * The DiskManager works with pages which is the chunk of bytes that contain data.
 * It does not understand rows/columns etc in the database.
 */
class DiskManager {
 public:
  explicit DiskManager(const std::string &db_file_name);

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

  /**
   * @brief Allocates a new page in the database file.
   * @return The ID of the page to deallocate.
   */
  page_id_t allocate_page();

  /**
   * @brief Deallocates a page.
   * @param page_id The ID of the page to deallocate.
   */
  IOResult deallocate_page(page_id_t page_id);

 private:
  std::string file_name_;
  std::fstream db_file_;
};