//
// Created by Amit Chavan on 12/26/25.
//

#include <gtest/gtest.h>
#include <filesystem>
#include "storage/iam_manager.h"
#include "storage/disk_manager.h"
#include "storage/extent_manager.h"

namespace letty {

class IamManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_db_file = "test_iam_manager.db";
    std::filesystem::remove(test_db_file);
    
    disk_manager = std::make_unique<DiskManager>(test_db_file);
    extent_manager = std::make_unique<ExtentManager>(*disk_manager);
    iam_manager = std::make_unique<IamManager>(*disk_manager, *extent_manager);
  }

  void TearDown() override {
    std::filesystem::remove(test_db_file);
  }

  std::string test_db_file;
  std::unique_ptr<DiskManager> disk_manager;
  std::unique_ptr<ExtentManager> extent_manager;
  std::unique_ptr<IamManager> iam_manager;
};

TEST_F(IamManagerTest, TestCreateChain) {
  page_id_t iam_page_id = iam_manager->create_iam_chain();
  EXPECT_NE(iam_page_id, INVALID_PAGE_ID);
  
  // It should allocate a new extent.
  // Extent 0 is System (Header+GAM+IAM).
  // Extent 1 is the first allocatable extent.
  // So we expect page 8.
  EXPECT_EQ(iam_page_id, 8);

  // Read back to verify it's an IAM page
  char buffer[PAGE_SIZE];
  disk_manager->read_page(iam_page_id, buffer);
  auto page = reinterpret_cast<SparseIamPage*>(buffer);
  
  EXPECT_EQ(page->next_bitmap_page_id, INVALID_PAGE_ID);
}

TEST_F(IamManagerTest, TestAllocation) {
  // 1. Create a chain
  page_id_t head_node = iam_manager->create_iam_chain();
  ASSERT_EQ(head_node, 8); // Extent 1

  // 2. Allocate an extent
  // Extent 0 = System
  // Extent 1 = This IAM Chain
  // Next free should be Extent 2 (Page 16)
  page_id_t allocated_page = iam_manager->allocate_extent(head_node);
  EXPECT_EQ(allocated_page, 16);

  // 3. Verify in IAM page
  char buffer[PAGE_SIZE];
  disk_manager->read_page(head_node, buffer);
  auto page = reinterpret_cast<SparseIamPage*>(buffer);
  
  // Extent 2 corresponds to index 2 in bitmap
  Bitmap bitmap(page->bitmap, SPARSE_MAX_BITS);
  EXPECT_TRUE(bitmap.is_set(2));
  
  // Extent 0 and 1 should NOT be set in THIS IAM page technically.
  // Wait, Extent 0 and 1 are allocated globally.
  // Does this IAM page own Extent 0? No.
  // Does this IAM page own Extent 1 (itself)? No, ExtentManager owns it.
  // So Bit 0 and Bit 1 should be 0.
  EXPECT_FALSE(bitmap.is_set(0)); 
  EXPECT_FALSE(bitmap.is_set(1));
}

} // namespace letty
