//
// Created by Amit Chavan on 8/30/25.
//

#include "disk_manager.h"
#include "storage/config.h"
#include <iostream>
#include <cassert>
#include <utility>

namespace letty {
DiskManager::DiskManager(std::string db_file) : file_name_(std::move(db_file)) {
  assert(!file_name_.empty() && "Database file path cannot be empty");

  // Open the file with flags for reading, writing, and binary mode.
  db_file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);

  // If the file cannot be opened (e.g., because it doesn't exist), we create it.
  if (!db_file_.is_open()) {

	// The `trunc` flag creates file if it does not exist.
	db_file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	if (!db_file_.is_open()) {
	  // We can't proceed without db file.
	  throw std::runtime_error("FATAL: Failed to create or open database file: " + file_name_);
	}
  }
}

DiskManager::~DiskManager() {
  if (db_file_.is_open()) {
	db_file_.flush(); // flush to disk
	db_file_.close(); // close the db file
  }
}

IOResult DiskManager::write_page(page_id_t page_id, const char *page_data) {
  if (!db_file_.is_open()) {
	return IOResult::FILE_NOT_OPEN;
  }

  std::streampos offset = static_cast<std::streampos>(page_id) * PAGE_SIZE;
  db_file_.seekp(offset, std::ios::beg);
  if (db_file_.fail()) {
	db_file_.clear(); // Clear error flags to allow further operations
	return IOResult::SEEK_ERROR;
  }

  // Write exactly PAGE_SIZE bytes from the buffer to the file.
  db_file_.write(page_data, PAGE_SIZE);
  if (db_file_.fail()) {
	db_file_.clear();
	return IOResult::WRITE_ERROR;
  }

  // Ensure the data is physically written to disk and not just kept in an OS buffer.
  // This is crucial for database durability.
  db_file_.flush();
  return IOResult::SUCCESS;
}

IOResult DiskManager::read_page(letty::page_id_t page_id, char *page_data) {
  if (!db_file_.is_open()) {
	return IOResult::FILE_NOT_OPEN;
  }

  // Calculate the offset to seek to in the file.
  std::streampos offset = static_cast<std::streampos>(page_id) * PAGE_SIZE;

  // Move the file's "get" (read) pointer to the correct position.
  db_file_.seekg(offset, std::ios::beg);
  if (db_file_.fail()) {
	// This can happen if we try to read a page that doesn't exist yet.
	db_file_.clear(); // Clear error flags
	return IOResult::SEEK_ERROR;
  }

  // Read exactly PAGE_SIZE bytes from the file into the buffer.
  db_file_.read(page_data, PAGE_SIZE);
  if (db_file_.fail()) {
	// `gcount()` returns the number of bytes actually read.
	// This check is useful for diagnosing reads past the end of the file.
	db_file_.clear();
	return IOResult::READ_ERROR;
  }
  return IOResult::SUCCESS;
}
}

