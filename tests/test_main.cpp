//
// Custom test main with logging initialization
//

#include <gtest/gtest.h>
#include <chrono>
#include <unistd.h>
#include "common/logger.h"

int main(int argc, char** argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize logging for tests - appends to minidb_test.log
    try {
        minidb::Logger::init_for_tests("minidb_test.log");
        
        // Log which test we're running (if a single test is specified)
        std::string filter = ::testing::GTEST_FLAG(filter);
        if (!filter.empty() && filter != "*") {
            LOG_INFO("=== Running test filter: {} ===", filter);
        } else {
            LOG_INFO("=== MiniDB Full Test Suite Started ===");
        }
        LOG_INFO("Total tests available: {}", ::testing::UnitTest::GetInstance()->total_test_count());
        
        // Run all tests
        int result = RUN_ALL_TESTS();
        
        LOG_INFO("=== MiniDB Test Suite Completed ===");
        LOG_INFO("Test result: {}", (result == 0) ? "PASSED" : "FAILED");
        
        // Cleanup logging
        minidb::Logger::shutdown();
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logging: " << e.what() << std::endl;
        // Still run tests even if logging fails
        return RUN_ALL_TESTS();
    }
}