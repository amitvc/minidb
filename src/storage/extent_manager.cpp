//
// Created by Amit Chavan on 9/15/25.
//

#include "storage_def.h"

#include "extent_manager.h"


page_id_t ExtentManager::allocate_extent() {

}

void ExtentManager::deallocate_extent(page_id_t start_page_id) {

}

void ExtentManager::initialize_new_db() {
	// Let's implement this function.
	char buffer[PAGE_SIZE];
	memset(buffer,0, sizeof(buffer));
	auto header = new(buffer) DatabaseHeader();
	// Allocate GAM and IAM page.
	header->total_pages = 2;
  	disk_manager->write_page(HEADER_PAGE_ID, buffer);
	char gam_page_buffer[PAGE_SIZE];
  	memset(gam_page_buffer,0, sizeof(gam_page_buffer));
	auto gam_page = new (gam_page_buffer)BitmapPage();
	gam_page->page_type = PageType::GAM;
	

}


ExtentManager::ExtentManager(DiskManager *disk_manager) {
	this->disk_manager = disk_manager;
}
