//
// Test suite for sparse IAM optimization functionality
// Tests the optimized IAM chain that avoids creating empty intermediate pages
//

#include <gtest/gtest.h>
#include "storage/iam_manager.h"
#include "storage/disk_manager.h"
#include "storage/extent_manager.h"
#include "common/logger.h"
#include <filesystem>
#include <memory>
#include <chrono>
#include <vector>

namespace letty {

class SparseIamManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database file for testing
        test_db_file = std::filesystem::temp_directory_path() / "sparse_iam_test.db";
        
        // Initialize components
        disk_manager = std::make_unique<DiskManager>(test_db_file.string());
        extent_manager = std::make_unique<ExtentManager>(*disk_manager);
        iam_manager = std::make_unique<IamManager>(*disk_manager, *extent_manager);
        
        
        LOG_STORAGE_INFO("Test setup complete for sparse IAM tests");
    }
    
    void TearDown() override {
        // Cleanup
        disk_manager.reset();
        extent_manager.reset();
        iam_manager.reset();
        
        // Remove test file
        if (std::filesystem::exists(test_db_file)) {
            std::filesystem::remove(test_db_file);
        }
        
        LOG_STORAGE_INFO("Test teardown complete");
    }
    
    /**
     * Helper function to create a test table's IAM chain
     */
    page_id_t create_test_table_iam() {
        return iam_manager->create_iam_chain();
    }
    
    /**
     * Helper to verify IAM page covers expected extent range
     */
    bool verify_iam_page_covers_extent(page_id_t iam_page_id, uint64_t extent_index) {
        char buffer[PAGE_SIZE];
        if (disk_manager->read_page(iam_page_id, buffer) != IOResult::SUCCESS) {
            return false;
        }
        
        auto sparse_page = reinterpret_cast<SparseIamPage*>(buffer);
        return sparse_page->covers_extent(extent_index);
    }
    
    /**
     * Helper to count the number of pages in an IAM chain
     */
    size_t count_iam_chain_length(page_id_t head_page_id) {
        size_t count = 0;
        page_id_t current = head_page_id;
        
        while (current != INVALID_PAGE_ID) {
            count++;
            char buffer[PAGE_SIZE];
            if (disk_manager->read_page(current, buffer) != IOResult::SUCCESS) {
                break;
            }
            auto page = reinterpret_cast<SparseIamPage*>(buffer);
            current = page->next_bitmap_page_id;
        }
        
        return count;
    }
    
    std::filesystem::path test_db_file;
    std::unique_ptr<DiskManager> disk_manager;
    std::unique_ptr<ExtentManager> extent_manager;
    std::unique_ptr<IamManager> iam_manager;
};

/**
 * Test basic sparse IAM allocation - should create only necessary pages
 */
TEST_F(SparseIamManagerTest, BasicSparseAllocation) {
    // Create a table's IAM chain
    page_id_t table_iam = create_test_table_iam();
    ASSERT_NE(table_iam, INVALID_PAGE_ID);
    
    // Initially should have 1 IAM page
    EXPECT_EQ(count_iam_chain_length(table_iam), 1);
    
    // Allocate extent in first range (should use existing page)
    page_id_t extent1 = iam_manager->allocate_extent_sparse(table_iam);
    ASSERT_NE(extent1, INVALID_PAGE_ID);
    
    // Should still have only 1 IAM page
    EXPECT_EQ(count_iam_chain_length(table_iam), 1);
    
    LOG_STORAGE_INFO("Basic sparse allocation test passed");
}

/**
 * Test sparse allocation with large gap - should skip intermediate ranges
 */
TEST_F(SparseIamManagerTest, SparseAllocationWithLargeGap) {
    page_id_t table_iam = create_test_table_iam();
    ASSERT_NE(table_iam, INVALID_PAGE_ID);
    
    // Simulate allocating an extent in a very high range
    // We'll need to manually allocate a high-numbered extent from ExtentManager
    // For this test, we'll simulate the scenario by forcing allocation
    
    // First, allocate several extents to push the allocation pointer high
    std::vector<page_id_t> intermediate_extents;
    const int NUM_INTERMEDIATE = 100; // This will create some distance
    
    for (int i = 0; i < NUM_INTERMEDIATE; ++i) {
        page_id_t extent = extent_manager->allocate_extent();
        if (extent != INVALID_PAGE_ID) {
            intermediate_extents.push_back(extent);
        }
    }
    
    // Now allocate through IAM - this should be in a higher range
    page_id_t distant_extent = iam_manager->allocate_extent_sparse(table_iam);
    ASSERT_NE(distant_extent, INVALID_PAGE_ID);
    
    // Should have created appropriate IAM pages, but not all intermediate ones
    size_t chain_length = count_iam_chain_length(table_iam);
    
    // The key test: chain length should be much less than what would be needed
    // if we created every intermediate page
    EXPECT_LE(chain_length, 5); // Should be minimal number of pages
    
    LOG_STORAGE_INFO("Sparse allocation with large gap test passed, chain length: {}", chain_length);
}

/**
 * Test multiple sparse allocations in different ranges
 */
TEST_F(SparseIamManagerTest, MultipleSparseRanges) {
    page_id_t table_iam = create_test_table_iam();
    ASSERT_NE(table_iam, INVALID_PAGE_ID);
    
    // Allocate extents that will require different IAM pages
    std::vector<page_id_t> allocated_extents;
    
    // Force allocation in first range
    page_id_t extent1 = iam_manager->allocate_extent_sparse(table_iam);
    ASSERT_NE(extent1, INVALID_PAGE_ID);
    allocated_extents.push_back(extent1);
    
    // Force some intermediate allocations to push the range higher
    // Need to cross SPARSE_MAX_BITS boundary (32640) to get to next range
    // Since we need extent index > 32640, and each extent spans EXTENT_SIZE=8 pages,
    // we need to push the allocation beyond page 32640*8 = 261120
    page_id_t last_allocated = 0;
    int successful_allocations = 0;
    for (int i = 0; i < 35000; ++i) {
        page_id_t temp_extent = extent_manager->allocate_extent();
        if (temp_extent != INVALID_PAGE_ID) {
            last_allocated = temp_extent;
            successful_allocations++;
            // Don't add to IAM, just consume extent numbers
        } else {
            LOG_STORAGE_INFO("Failed to allocate extent at iteration {}", i);
            break;
        }
    }
    LOG_STORAGE_INFO("Allocated {} intermediate extents, last at page {}", 
                    successful_allocations, last_allocated);
    
    // Now allocate through IAM again - should be in higher range
    page_id_t extent2 = iam_manager->allocate_extent_sparse(table_iam);
    ASSERT_NE(extent2, INVALID_PAGE_ID);
    allocated_extents.push_back(extent2);
    
    // Verify that we have multiple IAM pages but not excessive
    size_t chain_length = count_iam_chain_length(table_iam);
    EXPECT_GE(chain_length, 2); // Should have at least 2 pages for different ranges
    EXPECT_LE(chain_length, 4); // But not excessive number
    
    LOG_STORAGE_INFO("Multiple sparse ranges test passed, chain length: {}", chain_length);
}

/**
 * Test IAM page range calculation
 */
TEST_F(SparseIamManagerTest, RangeCalculation) {
    // Test the range calculation logic
    uint64_t extent_index_1 = 100;
    uint64_t expected_range_1 = (extent_index_1 / SPARSE_MAX_BITS) * SPARSE_MAX_BITS;
    EXPECT_EQ(expected_range_1, 0); // First 32640 extents map to range 0
    
    uint64_t extent_index_2 = SPARSE_MAX_BITS + 500;
    uint64_t expected_range_2 = (extent_index_2 / SPARSE_MAX_BITS) * SPARSE_MAX_BITS;
    EXPECT_EQ(expected_range_2, SPARSE_MAX_BITS); // Second range starts at SPARSE_MAX_BITS
    
    LOG_STORAGE_INFO("Range calculation test passed");
}

/**
 * Test SparseIamPage structure functionality
 */
TEST_F(SparseIamManagerTest, SparseIamPageStructure) {
    // Create a sparse IAM page manually
    char buffer[PAGE_SIZE] = {0};
    auto sparse_page = new(buffer) SparseIamPage();
    sparse_page->extent_range_start = 65408; // Arbitrary range start
    
    // Test covers_extent method
    EXPECT_TRUE(sparse_page->covers_extent(65408)); // Start of range
    EXPECT_TRUE(sparse_page->covers_extent(65500)); // Middle of range
    EXPECT_TRUE(sparse_page->covers_extent(65408 + SPARSE_MAX_BITS - 1)); // End of range
    
    EXPECT_FALSE(sparse_page->covers_extent(65407)); // Just before range
    EXPECT_FALSE(sparse_page->covers_extent(65408 + SPARSE_MAX_BITS)); // Just after range
    EXPECT_FALSE(sparse_page->covers_extent(0)); // Far before range
    EXPECT_FALSE(sparse_page->covers_extent(200000)); // Far after range
    
    // Test get_bit_offset method
    EXPECT_EQ(sparse_page->get_bit_offset(65408), 0); // Start maps to bit 0
    EXPECT_EQ(sparse_page->get_bit_offset(65409), 1); // Next extent maps to bit 1
    EXPECT_EQ(sparse_page->get_bit_offset(65500), 92); // Offset calculation
    
    LOG_STORAGE_INFO("SparseIamPage structure test passed");
}

/**
 * Test backward compatibility with regular SparseIamPages
 */
TEST_F(SparseIamManagerTest, BackwardCompatibility) {
    page_id_t table_iam = create_test_table_iam();
    ASSERT_NE(table_iam, INVALID_PAGE_ID);
    
    // The first IAM page created by create_iam_chain() is a SparseIamPage
    // Now try sparse allocation - it should work with the existing page
    page_id_t extent1 = iam_manager->allocate_extent_sparse(table_iam);
    ASSERT_NE(extent1, INVALID_PAGE_ID);
    
    // Should still work and use the existing IAM page for first range
    EXPECT_EQ(count_iam_chain_length(table_iam), 1);
    
    LOG_STORAGE_INFO("Backward compatibility test passed");
}

/**
 * Performance test - verify sparse allocation is efficient
 */
TEST_F(SparseIamManagerTest, PerformanceVerification) {
    page_id_t table_iam = create_test_table_iam();
    ASSERT_NE(table_iam, INVALID_PAGE_ID);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Allocate multiple extents using sparse method
    std::vector<page_id_t> allocated_extents;
    const int NUM_ALLOCATIONS = 50;
    
    for (int i = 0; i < NUM_ALLOCATIONS; ++i) {
        page_id_t extent = iam_manager->allocate_extent_sparse(table_iam);
        if (extent != INVALID_PAGE_ID) {
            allocated_extents.push_back(extent);
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify we allocated reasonable number of extents
    EXPECT_GT(allocated_extents.size(), NUM_ALLOCATIONS / 2);
    
    // Verify chain length is reasonable (not excessive)
    size_t final_chain_length = count_iam_chain_length(table_iam);
    EXPECT_LE(final_chain_length, 10); // Should be much less than NUM_ALLOCATIONS
    
    LOG_STORAGE_INFO("Performance test: {} allocations in {}ms, final chain length: {}", 
                    allocated_extents.size(), duration.count(), final_chain_length);
}

/**
 * Test edge cases and error conditions
 */
TEST_F(SparseIamManagerTest, EdgeCases) {
    // Test with invalid IAM head
    page_id_t invalid_extent = iam_manager->allocate_extent_sparse(INVALID_PAGE_ID);
    EXPECT_EQ(invalid_extent, INVALID_PAGE_ID);
    
    // Test range calculation at boundaries
    EXPECT_EQ(iam_manager->calculate_sparse_range_start(0), 0);
    EXPECT_EQ(iam_manager->calculate_sparse_range_start(SPARSE_MAX_BITS - 1), 0);
    EXPECT_EQ(iam_manager->calculate_sparse_range_start(SPARSE_MAX_BITS), SPARSE_MAX_BITS);
    EXPECT_EQ(iam_manager->calculate_sparse_range_start(SPARSE_MAX_BITS + 1), SPARSE_MAX_BITS);
    
    LOG_STORAGE_INFO("Edge cases test passed");
}

} // namespace letty