//
// Main entry point for Letty CLI application
//

#include "cli/LettyCli.h"
#include "common/logger.h"
#include <iostream>

int main() {
    try {
        // Initialize logging system
        letty::Logger::init("letty.log");
        
        LOG_INFO("=== Letty Started ===");
        LOG_INFO("Version: 1.0.0");
        LOG_INFO("Build: " __DATE__ " " __TIME__);
        
        // Run the CLI
        letty::LettyCli cli;
        cli.Run();
        
        LOG_INFO("=== Letty Shutting Down ===");
        
        // Cleanup logging
        letty::Logger::shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}