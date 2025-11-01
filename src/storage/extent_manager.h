//
// Created by Amit Chavan on 9/15/25.
//

#pragma once

#include "disk_manager.h"

/**
 * @class ExtentManager
 * @brief Manages allocation and deallocation of Extent's within the database file.
 * An Extent is a collection of 1 or more pages. All pages within an Extent typically
 * belong to a single database table.
 * Can read more about them here - https://tinyurl.com/32rhava7
 */
class ExtentManager {
  explicit ExtentManager(DiskManager* disk_manager);

  page_id_t allocate_extent();

  void deallocate_extent(page_id_t start_page_id);
 private:
  /**
   * @brief Initializes a brand new database file.
   * This function is called by the constructor if it detects that the database
   * file is empty. It creates and writes the initial Header and GAM pages.
   */
  void initialize_new_db();

  DiskManager* disk_manager;

  std::mutex lock;
};

