//
// Created by Amit Chavan on 6/6/25.
//

#pragma once
#include <string>

namespace minidb {
    /**
     * @class MiniDBCli
     * @brief Interactive command-line interface for the MiniDB database
     * 
     * The MiniDBCli class provides a simple command-line interface that allows
     * users to interact with the MiniDB database. It handles user input, processes
     * SQL commands, and displays results in a user-friendly format.
     * 
     * @par Features:
     * - Interactive prompt for SQL commands
     * - Command processing and validation
     * - Error handling and user feedback
     * - Session management
     * 
     * @par Usage Example:
     * @code
     * MiniDBCli cli;
     * cli.Run();  // Starts the interactive session
     * @endcode
     */
    class MiniDBCli {
    public:
        /**
         * @brief Starts the interactive CLI session
         * 
         * This method begins the main command loop, displaying prompts and
         * processing user input until the session is terminated.
         */
        void Run();

    private:
        /**
         * @brief Processes a single SQL command from user input
         * @param input The raw SQL command string entered by the user
         */
        void ProcessCommand(const std::string &input);

        /**
         * @brief Generates and returns the command prompt string
         * @return String containing the prompt to display to the user
         */
        std::string Prompt();

    };
}


