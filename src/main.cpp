//
// Main entry point for MiniDB CLI application
//

#include "cli/MiniDBCli.h"
#include "common/logger.h"
#include <iostream>

int main() {
    try {
        // Initialize logging system
        minidb::Logger::init("minidb.log");
        
        LOG_INFO("=== MiniDB Started ===");
        LOG_INFO("Version: 1.0.0");
        LOG_INFO("Build: " __DATE__ " " __TIME__);
        
        // Run the CLI
        minidb::MiniDBCli cli;
        cli.Run();
        
        LOG_INFO("=== MiniDB Shutting Down ===");
        
        // Cleanup logging
        minidb::Logger::shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}