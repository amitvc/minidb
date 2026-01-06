//
// Created by Amit Chavan on 12/11/25.
//
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <vector>
#include <set>
#include "storage/extent_manager.h"
#include "storage/disk_manager.h"
#include "storage/storage_def.h"

namespace minidb {

class ExtentManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
	test_db_file = "test_extent_manager.db";
	if (std::filesystem::exists(test_db_file)) {
	  std::filesystem::remove(test_db_file);
	}
  }

  void TearDown() override {
	std::filesystem::remove(test_db_file);
  }

  std::string test_db_file;
};

// Test if the db initialization works correctly when we start with empty db file.
// This test ensures all essential pages (db header, GAM's and IAM's) are written correctly to the db file.
TEST_F(ExtentManagerTest, TestInitialization) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Check if Header page is initialized correctly
  char header_buffer[PAGE_SIZE];
  disk_manager->read_page(HEADER_PAGE_ID, header_buffer);
  auto *header = reinterpret_cast<DatabaseHeader *>(header_buffer);
  EXPECT_EQ(std::string(header->signature), DB_SIGNATURE);
  EXPECT_EQ(header->total_pages, EXTENT_SIZE);
  EXPECT_EQ(header->gam_page_id, FIRST_GAM_PAGE_ID);
  EXPECT_EQ(header->sys_tables_iam_page, SYS_TABLES_IAM_PAGE_ID);
  EXPECT_EQ(header->sys_columns_iam_page, SYS_COLUMNS_IAM_PAGE_ID);

  // Check if GAM page is initialized
  char gam_buffer[PAGE_SIZE];
  disk_manager->read_page(FIRST_GAM_PAGE_ID, gam_buffer);
  auto *gam_page = reinterpret_cast<BitmapPage *>(gam_buffer);
  EXPECT_EQ(gam_page->page_type, PageType::GAM);

  // Verify extent 0 is allocated (it holds Header, GAM, IAM)
  Bitmap gam_bitmap(gam_page->bitmap, (PAGE_SIZE - 8) * 8);
  EXPECT_TRUE(gam_bitmap.is_set(0));

  char iam_buffer[PAGE_SIZE] = {0};
  disk_manager->read_page(SYS_TABLES_IAM_PAGE_ID, iam_buffer);
  auto *iam_page2 = reinterpret_cast<BitmapPage *>(iam_buffer);
  EXPECT_EQ(iam_page2->page_type, PageType::IAM);

  memset(iam_buffer, 0, PAGE_SIZE);

  disk_manager->read_page(SYS_COLUMNS_IAM_PAGE_ID, iam_buffer);
  auto *iam_page3 = reinterpret_cast<BitmapPage *>(iam_buffer);
  EXPECT_EQ(iam_page3->page_type, PageType::IAM);
}

// Verify ExtentManager allocates pages correctly.
TEST_F(ExtentManagerTest, TestAllocation) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Allocate first available extent (Should be extent 1, since extent 0 is system)
  page_id_t extent1_start = extent_manager->allocate_extent();
  EXPECT_EQ(extent1_start, 1 * EXTENT_SIZE); // 8

  // Allocate second extent
  page_id_t extent2_start = extent_manager->allocate_extent();
  EXPECT_EQ(extent2_start, 2 * EXTENT_SIZE); // 16

  // Verify in GAM
  char gam_buffer[PAGE_SIZE];
  disk_manager->read_page(FIRST_GAM_PAGE_ID, gam_buffer);
  auto *gam_page = reinterpret_cast<BitmapPage *>(gam_buffer);
  Bitmap gam_bitmap(gam_page->bitmap, (PAGE_SIZE - 8) * 8);
  //bit 1 and 2 is set to 1 to indicate extents 1 and 2 are allocated.
  EXPECT_TRUE(gam_bitmap.is_set(1));
  EXPECT_TRUE(gam_bitmap.is_set(2));
}

TEST_F(ExtentManagerTest, TestPersistence) {
  {
	auto disk_manager = std::make_unique<DiskManager>(test_db_file);
	auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

	// Allocate alloc 1
	page_id_t p1 = extent_manager->allocate_extent();
	EXPECT_EQ(p1, 8);
  }

  // Re-open
  {
	auto disk_manager = std::make_unique<DiskManager>(test_db_file);
	auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

	// Next allocation should be extent 2 (page 16), keeping extent 1 allocated
	page_id_t p2 = extent_manager->allocate_extent();
	EXPECT_EQ(p2, 16);

	// Check that extent 1 is still allocated
	char gam_buffer[PAGE_SIZE];
	disk_manager->read_page(FIRST_GAM_PAGE_ID, gam_buffer);
	auto *gam_page = reinterpret_cast<BitmapPage *>(gam_buffer);
	Bitmap gam_bitmap(gam_page->bitmap, (PAGE_SIZE - 8) * 8);
	EXPECT_TRUE(gam_bitmap.is_set(1));
	EXPECT_TRUE(gam_bitmap.is_set(2));
  }
}

TEST_F(ExtentManagerTest, TestGAMPageExpansion) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  //  Manually fill the first GAM page
  char gam_buffer[PAGE_SIZE];
  disk_manager->read_page(FIRST_GAM_PAGE_ID, gam_buffer);
  auto *gam_page = reinterpret_cast<BitmapPage *>(gam_buffer);

  // Fill all bits
  std::memset(gam_page->bitmap, 0xFF, sizeof(gam_page->bitmap));

  disk_manager->write_page(FIRST_GAM_PAGE_ID, gam_buffer);

  // Allocate extent. This should trigger creating a new GAM page.

  page_id_t new_extent_page_id = extent_manager->allocate_extent();

  // Get header to check total pages
  char header_buffer[PAGE_SIZE];
  disk_manager->read_page(HEADER_PAGE_ID, header_buffer);

  auto *header = reinterpret_cast<DatabaseHeader *>(header_buffer);

  // Total pages should NOT have increased, because we packed the new GAM into Extent 0 (Page 4).
  // Initial: 8 (Extent 0).
  EXPECT_EQ(header->total_pages, EXTENT_SIZE);

  // The new GAM page should be at Page 4.
  disk_manager->read_page(FIRST_GAM_PAGE_ID, gam_buffer);
  EXPECT_EQ(gam_page->next_bitmap_page_id, 4); // Should point to Page 4

  // Validate the new GAM page itself
  char new_gam_buffer[PAGE_SIZE];
  disk_manager->read_page(4, new_gam_buffer);

  auto *new_gam_page = reinterpret_cast<BitmapPage *>(new_gam_buffer);

  EXPECT_EQ(new_gam_page->page_type, PageType::GAM);
  // Validate allocation return value
  // We expect a valid page_id > 0.

  // 4088 * 8 * 8 [ total number of bytes * bits per byte * EXTENT_SIZE]
  // 4088 => 4096 - 4 - 4. See bitmapPage for details.
  EXPECT_EQ(new_extent_page_id, 4088 * 8 * 8);

}

TEST_F(ExtentManagerTest, TestDeallocation) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Allocate two extents
  page_id_t extent1_start = extent_manager->allocate_extent(); // 8
  page_id_t extent2_start = extent_manager->allocate_extent(); // 16
  EXPECT_EQ(extent1_start, 8);
  EXPECT_EQ(extent2_start, 16);

  // Deallocate the first one
  extent_manager->deallocate_extent(extent1_start);

  // Verify in GAM that bit is cleared
  char gam_buffer[PAGE_SIZE];
  disk_manager->read_page(FIRST_GAM_PAGE_ID, gam_buffer);
  auto *gam_page = reinterpret_cast<BitmapPage *>(gam_buffer);

  Bitmap gam_bitmap(gam_page->bitmap, MAX_BITS);

  EXPECT_FALSE(gam_bitmap.is_set(1)); // Extent 1 should be free
  EXPECT_TRUE(gam_bitmap.is_set(2));  // Extent 2 should still be allocated

  // Now, allocate again - should reuse the deallocated one
  page_id_t reused_extent_start = extent_manager->allocate_extent();
  EXPECT_EQ(reused_extent_start, extent1_start);
}

TEST_F(ExtentManagerTest, TestGAMPageExpansionWhenExtent0IsFull) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // We need to fill up Extent 0 with GAM pages.
  // Extent 0 layout:
  // 0: Header
  // 1: GAM 0
  // 2: IAM (sys_tables)
  // 3: IAM (sys_columns)

  page_id_t gam_pages[] = {1, 4, 5, 6, 7};
  int num_gam_pages = 5;

  char buffer[PAGE_SIZE];

  for (int i = 0; i < num_gam_pages; ++i) {
	page_id_t current = gam_pages[i];
	page_id_t next = (i == num_gam_pages - 1) ? INVALID_PAGE_ID : gam_pages[i + 1];

	// Read or Init
	std::memset(buffer, 0, PAGE_SIZE);
	auto *page = reinterpret_cast<BitmapPage *>(buffer);
	page->page_type = PageType::GAM;
	page->next_bitmap_page_id = next;

	// Fill bitmap
	std::memset(page->bitmap, 0xFF, sizeof(page->bitmap));

	disk_manager->write_page(current, buffer);
  }

  // Verify Header before
  char header_buffer[PAGE_SIZE];
  disk_manager->read_page(HEADER_PAGE_ID, header_buffer);
  auto *header = reinterpret_cast<DatabaseHeader *>(header_buffer);
  EXPECT_EQ(header->total_pages, EXTENT_SIZE);

  // Allocate!
  // Should traverse 1->3->4->5->6->7->Full -> Create New at 8.
  page_id_t new_page_id = extent_manager->allocate_extent();

  // Verify Header after
  disk_manager->read_page(HEADER_PAGE_ID, header_buffer);
  header = reinterpret_cast<DatabaseHeader *>(header_buffer);
  // Should have expanded by one extent (8 pages)
  EXPECT_EQ(header->total_pages, EXTENT_SIZE + EXTENT_SIZE); // 16

  // Verify New GAM at Page 8
  char new_gam_buffer[PAGE_SIZE];
  IOResult res = disk_manager->read_page(8, new_gam_buffer);
  EXPECT_EQ(res, IOResult::SUCCESS);
  auto *new_gam = reinterpret_cast<BitmapPage *>(new_gam_buffer);
  EXPECT_EQ(new_gam->page_type, PageType::GAM);
  EXPECT_EQ(new_gam->next_bitmap_page_id, INVALID_PAGE_ID);

  // Verify Link from Page 7
  char page7_buffer[PAGE_SIZE];
  disk_manager->read_page(7, page7_buffer);
  auto *page7 = reinterpret_cast<BitmapPage *>(page7_buffer);
  EXPECT_EQ(page7->next_bitmap_page_id, 8);

  // Return ID should be valid and huge
  EXPECT_GT(new_page_id, 0);
}

TEST_F(ExtentManagerTest, TestCorruptDatabaseSignature) {
  // 1. Create a file with invalid signature
  {
	auto disk_manager = std::make_unique<DiskManager>(test_db_file);
	// Initialize properly first to get file structure
	ExtentManager em(*disk_manager);
  }

  // 2. Corrupt the signature
  {
	std::fstream file(test_db_file, std::ios::in | std::ios::out | std::ios::binary);
	file.seekp(0);
	file.write("INVALID ", 8);
	file.close();
  }

  // 3. Try to open with ExtentManager - should throw
  {
	auto disk_manager = std::make_unique<DiskManager>(test_db_file);
	EXPECT_THROW({
				   ExtentManager em(*disk_manager);
				 }, std::runtime_error);
  }
}

TEST_F(ExtentManagerTest, TestConcurrentAllocationDeallocation) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  const int num_threads = 4;
  const int operations_per_thread = 10;
  std::vector<std::thread> threads;
  std::vector<std::vector<page_id_t>> allocated_extents(num_threads);

  // Launch allocation threads
  for (int i = 0; i < num_threads; ++i) {
	threads.emplace_back([&extent_manager, &allocated_extents, i, operations_per_thread]() {
	  for (int j = 0; j < operations_per_thread; ++j) {
		page_id_t extent_start = extent_manager->allocate_extent();
		EXPECT_NE(extent_start, INVALID_PAGE_ID);
		allocated_extents[i].push_back(extent_start);
	  }
	});
  }

  // Wait for allocation threads to complete
  for (auto &thread : threads) {
	thread.join();
  }
  threads.clear();

  // Verify all allocated extents are unique
  std::set < page_id_t > all_extents;
  for (int i = 0; i < num_threads; ++i) {
	for (page_id_t extent : allocated_extents[i]) {
	  EXPECT_TRUE(all_extents.insert(extent).second) << "Duplicate extent allocated: " << extent;
	}
  }

  // Launch deallocation threads
  for (int i = 0; i < num_threads; ++i) {
	threads.emplace_back([&extent_manager, &allocated_extents, i]() {
	  for (page_id_t extent : allocated_extents[i]) {
		extent_manager->deallocate_extent(extent);
	  }
	});
  }

  // Wait for deallocation threads to complete
  for (auto &thread : threads) {
	thread.join();
  }

  // Verify all extents can be reallocated (showing they were properly deallocated)
  std::set < page_id_t > reallocated_extents;
  for (int i = 0; i < num_threads * operations_per_thread; ++i) {
	page_id_t extent_start = extent_manager->allocate_extent();
	EXPECT_NE(extent_start, INVALID_PAGE_ID);
	reallocated_extents.insert(extent_start);
  }

  // Should be able to reallocate the same number of extents
  EXPECT_EQ(reallocated_extents.size(), num_threads * operations_per_thread);
}

TEST_F(ExtentManagerTest, TestInvalidExtentDeallocation) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Test deallocating invalid page IDs
  EXPECT_NO_THROW(extent_manager->deallocate_extent(INVALID_PAGE_ID));
  EXPECT_NO_THROW(extent_manager->deallocate_extent(999999)); // Very large invalid ID

  // Test deallocating non-extent-aligned page IDs
  EXPECT_NO_THROW(extent_manager->deallocate_extent(1)); // Not start of extent
  EXPECT_NO_THROW(extent_manager->deallocate_extent(9)); // Not start of extent
}

TEST_F(ExtentManagerTest, TestDoubleFreeExtent) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Allocate an extent
  page_id_t extent_start = extent_manager->allocate_extent();
  EXPECT_NE(extent_start, INVALID_PAGE_ID);

  // Deallocate it once
  EXPECT_NO_THROW(extent_manager->deallocate_extent(extent_start));

  // Deallocate it again (double-free) - should not crash
  EXPECT_NO_THROW(extent_manager->deallocate_extent(extent_start));

  // Verify we can still allocate and it reuses the same extent
  page_id_t reused_extent = extent_manager->allocate_extent();
  EXPECT_EQ(reused_extent, extent_start);
}

TEST_F(ExtentManagerTest, TestDeallocateSystemExtent) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // First allocate a normal extent to ensure system is working
  page_id_t normal_extent = extent_manager->allocate_extent();
  EXPECT_EQ(normal_extent, EXTENT_SIZE); // Should be extent 1 (page 8)

  // Try to deallocate system extent (extent 0)
  // This should not crash but behavior might be undefined
  EXPECT_NO_THROW(extent_manager->deallocate_extent(0));
}

TEST_F(ExtentManagerTest, TestGAMCacheEfficiency) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Allocate multiple extents to test cache behavior
  std::vector<page_id_t> allocated_extents;

  // First allocation should cache the GAM page
  page_id_t extent1 = extent_manager->allocate_extent();
  EXPECT_EQ(extent1, EXTENT_SIZE);
  allocated_extents.push_back(extent1);

  // Subsequent allocations in the same GAM should hit cache
  for (int i = 0; i < 5; ++i) {
	page_id_t extent = extent_manager->allocate_extent();
	EXPECT_NE(extent, INVALID_PAGE_ID);
	allocated_extents.push_back(extent);
  }

  // Mix allocation and deallocation to test cache coherency
  extent_manager->deallocate_extent(allocated_extents[1]);

  // Allocate again - should reuse the deallocated extent (cache hit)
  page_id_t reused = extent_manager->allocate_extent();
  EXPECT_EQ(reused, allocated_extents[1]);

  // Deallocate an extent and then allocate from a different GAM
  // (when we force GAM expansion) to test cache miss scenarios
  extent_manager->deallocate_extent(allocated_extents[2]);
}

TEST_F(ExtentManagerTest, TestAllocationLimits) {
  auto disk_manager = std::make_unique<DiskManager>(test_db_file);
  auto extent_manager = std::make_unique<ExtentManager>(*disk_manager);

  // Allocate many extents to test system behavior under pressure
  std::vector<page_id_t> allocated_extents;

  // Allocate a reasonable number of extents (not enough to fill disk)
  // This tests that the system can handle many allocations
  for (int i = 0; i < 100; ++i) {
	page_id_t extent = extent_manager->allocate_extent();
	if (extent == INVALID_PAGE_ID) {
	  // If allocation fails, that's acceptable for high counts
	  break;
	}
	allocated_extents.push_back(extent);
  }

  // Should have allocated at least some extents
  EXPECT_GT(allocated_extents.size(), 0);

  // All allocated extents should be unique and properly aligned
  std::set < page_id_t > unique_extents(allocated_extents.begin(), allocated_extents.end());
  EXPECT_EQ(unique_extents.size(), allocated_extents.size());

  for (page_id_t extent : allocated_extents) {
	EXPECT_EQ(extent % EXTENT_SIZE, 0); // Should be extent-aligned
	EXPECT_GE(extent, EXTENT_SIZE); // Should not be system extent
  }

  // Cleanup - deallocate all extents
  for (page_id_t extent : allocated_extents) {
	EXPECT_NO_THROW(extent_manager->deallocate_extent(extent));
  }
}

}
