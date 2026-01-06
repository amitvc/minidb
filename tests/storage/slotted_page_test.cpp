#include <gtest/gtest.h>
#include "storage/slotted_page.h"
#include "storage/storage_def.h"

namespace minidb {

class SlottedPageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::memset(buffer, 0, PAGE_SIZE);
  }

  char buffer[PAGE_SIZE];
};

TEST_F(SlottedPageTest, Initialization) {
  SlottedPage page(buffer, true);
  
  EXPECT_EQ(page.get_num_slots(), 0);
  // Header size is small, so free space should be close to 4096
  EXPECT_GT(page.get_free_space(), 4000);
  EXPECT_LT(page.get_free_space(), 4096);
  
  auto* header = reinterpret_cast<SlottedPageHeader*>(buffer);
  EXPECT_EQ(header->page_type, PageType::Data);
  EXPECT_EQ(header->tuple_count, 0);
}

TEST_F(SlottedPageTest, InsertAndGetTuple) {
  SlottedPage page(buffer, true);
  
  std::string data1 = "Hello World";
  int32_t slot_id = page.insert_tuple(data1.c_str(), data1.size() + 1); // +1 for null terminator
  
  EXPECT_GE(slot_id, 0);
  EXPECT_EQ(page.get_num_slots(), 1);
  // Check tuple count manually via header
  auto* header = reinterpret_cast<SlottedPageHeader*>(buffer);
  EXPECT_EQ(header->tuple_count, 1);
  
  uint32_t size;
  char* retrieved_data = page.get_tuple(slot_id, &size);
  
  ASSERT_NE(retrieved_data, nullptr);
  EXPECT_EQ(size, data1.size() + 1);
  EXPECT_STREQ(retrieved_data, data1.c_str());
}

TEST_F(SlottedPageTest, MultipleInserts) {
  SlottedPage page(buffer, true);
  
  std::vector<std::string> strings = {"One", "Two", "Three", "Four"};
  std::vector<int32_t> slot_ids;
  
  for (const auto& s : strings) {
    int32_t id = page.insert_tuple(s.c_str(), s.size() + 1);
    EXPECT_GE(id, 0);
    slot_ids.push_back(id);
  }
  
  EXPECT_EQ(page.get_num_slots(), 4);
  
  // Verify in reverse order (just to be sure)
  for (int i = 0; i < 4; ++i) {
    uint32_t size;
    char* data = page.get_tuple(slot_ids[i], &size);
    EXPECT_STREQ(data, strings[i].c_str());
  }
}

TEST_F(SlottedPageTest, DeleteTuple) {
  SlottedPage page(buffer, true);
  
  int32_t id = page.insert_tuple("To Be Deleted", 14);
  EXPECT_GE(id, 0);
  
  EXPECT_TRUE(page.delete_tuple(id));
  
  // Should return nullptr now
  EXPECT_EQ(page.get_tuple(id, nullptr), nullptr);
  
  // Slots count remains same (logical delete)
  EXPECT_EQ(page.get_num_slots(), 1); 
  
  auto* header = reinterpret_cast<SlottedPageHeader*>(buffer);
  EXPECT_EQ(header->tuple_count, 0);
}

TEST_F(SlottedPageTest, PageFull) {
  SlottedPage page(buffer, true);
  
  // Try to insert something huge
  char huge_data[4000];
  std::memset(huge_data, 'A', 4000);
  
  int32_t id1 = page.insert_tuple(huge_data, 4000);
  EXPECT_GE(id1, 0);
  
  // Try to insert another huge thing - should fail
  int32_t id2 = page.insert_tuple(huge_data, 100); // Even small won't fit as 4000+header+slots > 4096
  EXPECT_EQ(id2, -1);
}

TEST_F(SlottedPageTest, ReuseSlot) {
  SlottedPage page(buffer, true);
  
  // Insert 3 items
  int32_t id1 = page.insert_tuple("Tuple 1", 8);
  int32_t id2 = page.insert_tuple("Tuple 2", 8);
  int32_t id3 = page.insert_tuple("Tuple 3", 8);
  
  EXPECT_EQ(id1, 0);
  EXPECT_EQ(id2, 1);
  EXPECT_EQ(id3, 2);
  EXPECT_EQ(page.get_num_slots(), 3);

  // Delete middle one
  page.delete_tuple(id2);
  EXPECT_EQ(page.get_tuple(id2, nullptr), nullptr);

  EXPECT_EQ(page.get_tuple_count(), 2); // should not include deleted slots
  EXPECT_EQ(page.get_num_slots(), 3); // deleted slots + active slots



  // Insert new one - should reuse slot 1 (id2)
  int32_t id4 = page.insert_tuple("Tuple 4", 8);
  EXPECT_EQ(id4, 1); // Should reuse slot 1
  
  EXPECT_EQ(page.get_num_slots(), 3); // Should not increase
  
  // Verify content
  uint32_t size;
  char* data = page.get_tuple(id4, &size);
  EXPECT_STREQ(data, "Tuple 4");
}
}
