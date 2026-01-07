//
// Simple integration test to verify sparse IAM functionality works
//

#include <gtest/gtest.h>
#include "storage/iam_manager.h"
#include "storage/disk_manager.h"
#include "storage/extent_manager.h"
#include "common/logger.h"
#include <filesystem>

namespace letty {

/**
 * Simple integration test for sparse IAM functionality
 */
TEST(IamIntegrationTest, SparseIamBasicFunctionality) {
    // Setup temporary database
    auto test_db_file = std::filesystem::temp_directory_path() / "iam_integration_test.db";
    
    try {
        // Initialize components
        DiskManager disk_manager(test_db_file.string());
        ExtentManager extent_manager(disk_manager);
        IamManager iam_manager(disk_manager, extent_manager);
        
        
        // Create a table's IAM chain
        page_id_t table_iam = iam_manager.create_iam_chain();
        ASSERT_NE(table_iam, INVALID_PAGE_ID);
        
        // Test sparse allocation
        page_id_t extent1 = iam_manager.allocate_extent_sparse(table_iam);
        EXPECT_NE(extent1, INVALID_PAGE_ID);
        
        // Test regular allocation for comparison
        page_id_t extent2 = iam_manager.allocate_extent(table_iam);
        EXPECT_NE(extent2, INVALID_PAGE_ID);
        
        // Both should be valid but different
        EXPECT_NE(extent1, extent2);
        
        LOG_STORAGE_INFO("Basic sparse IAM integration test passed");
        
    } catch (...) {
        // Cleanup
        if (std::filesystem::exists(test_db_file)) {
            std::filesystem::remove(test_db_file);
        }
        throw;
    }
    
    // Cleanup
    if (std::filesystem::exists(test_db_file)) {
        std::filesystem::remove(test_db_file);
    }
}

/**
 * Test SparseIamPage structure basic functionality
 */
TEST(IamIntegrationTest, SparseIamPageBasics) {
    // Test SparseIamPage structure
    SparseIamPage page;
    page.extent_range_start = 1000;
    
    // Test coverage check
    EXPECT_TRUE(page.covers_extent(1000));
    EXPECT_TRUE(page.covers_extent(1500));
    EXPECT_FALSE(page.covers_extent(999));
    EXPECT_FALSE(page.covers_extent(1000 + SPARSE_MAX_BITS));
    
    // Test bit offset calculation
    EXPECT_EQ(page.get_bit_offset(1000), 0);
    EXPECT_EQ(page.get_bit_offset(1001), 1);
    EXPECT_EQ(page.get_bit_offset(1010), 10);
    
    LOG_STORAGE_INFO("SparseIamPage basics test passed");
}

/**
 * Test range calculation function
 */
TEST(IamIntegrationTest, RangeCalculation) {
    auto test_db_file = std::filesystem::temp_directory_path() / "range_calc_test.db";
    
    try {
        DiskManager disk_manager(test_db_file.string());
        ExtentManager extent_manager(disk_manager);
        IamManager iam_manager(disk_manager, extent_manager);
        
        // Test range calculation
        EXPECT_EQ(iam_manager.calculate_sparse_range_start(0), 0);
        EXPECT_EQ(iam_manager.calculate_sparse_range_start(SPARSE_MAX_BITS - 1), 0);
        EXPECT_EQ(iam_manager.calculate_sparse_range_start(SPARSE_MAX_BITS), SPARSE_MAX_BITS);
        EXPECT_EQ(iam_manager.calculate_sparse_range_start(SPARSE_MAX_BITS + 100), SPARSE_MAX_BITS);
        
        LOG_STORAGE_INFO("Range calculation test passed");
        
    } catch (...) {
        if (std::filesystem::exists(test_db_file)) {
            std::filesystem::remove(test_db_file);
        }
        throw;
    }
    
    if (std::filesystem::exists(test_db_file)) {
        std::filesystem::remove(test_db_file);
    }
}

} // namespace letty