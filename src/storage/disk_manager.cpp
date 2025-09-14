//
// Created by Amit Chavan on 8/30/25.
//

#include "disk_manager.h"
#include "storage/config.h"
#include <iostream>

DiskManager::DiskManager(const std::string& db_file) : file_name_(db_file) {
  // Open the file with flags for reading, writing, and binary mode.
  db_file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);

  // If the file cannot be opened (e.g., because it doesn't exist), we create it.
  if (!db_file_.is_open()) {
	// The `trunc` flag creates the file if it doesn't exist.
	db_file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	//std::cout << "Absolute path: " << std::filesystem::absolute(file_name_) << std::endl;

	if (!db_file_.is_open()) {
	  // If it still fails, we have a serious problem (e.g., permissions).
	  std::cerr << "FATAL: Failed to create or open database file: " << file_name_ << std::endl;
	}
  }
}

DiskManager::~DiskManager() {
  if (db_file_.is_open()) {
	db_file_.close();
  }
}

IOResult DiskManager::write_page(page_id_t page_id, const char* page_data) {
  if (!db_file_.is_open()) {
	std::cerr << "Cannot write page. Database file is not open." << std::endl;
	return IOResult::FILE_NOT_OPEN;
  }

  std::streampos offset = static_cast<std::streampos>(page_id) * PAGE_SIZE;
  db_file_.seekp(offset, std::ios::beg);
  if (db_file_.fail()) {
	std::cerr << "Error seeking to page " << page_id << " for writing." << std::endl;
	db_file_.clear(); // Clear error flags to allow further operations
	return IOResult::SEEK_ERROR;
  }

  // Write exactly PAGE_SIZE bytes from the buffer to the file.
  db_file_.write(page_data, PAGE_SIZE);
  if (db_file_.fail()) {
	std::cerr << "Error writing to page " << page_id << "." << std::endl;
	db_file_.clear();
	return IOResult::WRITE_ERROR;
  }

  // Ensure the data is physically written to disk and not just kept in an OS buffer.
  // This is crucial for database durability.
  db_file_.flush();
  return IOResult::SUCCESS;
}

IOResult DiskManager::read_page(page_id_t page_id, char* page_data) {
  if (!db_file_.is_open()) {
	std::cerr << "Cannot read page. Database file is not open." << std::endl;
	return IOResult::FILE_NOT_OPEN;
  }

  // Calculate the offset to seek to in the file.
  std::streampos offset = static_cast<std::streampos>(page_id) * PAGE_SIZE;

  // Move the file's "get" (read) pointer to the correct position.
  db_file_.seekg(offset, std::ios::beg);
  if (db_file_.fail()) {
	// This can happen if we try to read a page that doesn't exist yet.
	std::cerr << "Error seeking to page " << page_id << " for reading. Page may not exist." << std::endl;
	db_file_.clear(); // Clear error flags
	return IOResult::SEEK_ERROR;
  }

  // Read exactly PAGE_SIZE bytes from the file into the buffer.
  db_file_.read(page_data, PAGE_SIZE);
  if (db_file_.fail()) {
	// `gcount()` returns the number of bytes actually read.
	// This check is useful for diagnosing reads past the end of the file.
	std::cerr << "Error reading from page " << page_id << ". Read " << db_file_.gcount() << " bytes." << std::endl;
	db_file_.clear();
	return IOResult::READ_ERROR;
  }

  return IOResult::SUCCESS;
}



