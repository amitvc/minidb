//
// Created by Amit Chavan on 6/6/25.
//

#include "MiniDBCli.h"
#include <iostream>
#include <algorithm>
#include <cctype>
namespace minidb {

    void MiniDBCli::Run() {
        std::string input;
        std::cout << "Welcome to MiniDB! Type 'exit' to quit.\n";

        while (true) {
            std::cout << Prompt();
            std::getline(std::cin, input);

            // Trim whitespace from both ends
            input.erase(input.begin(), std::find_if_not(input.begin(), input.end(), ::isspace));
            input.erase(std::find_if_not(input.rbegin(), input.rend(), ::isspace).base(), input.end());
            std::transform(input.begin(), input.end(), input.begin(), ::tolower);

            if (input == "exit") break;
            if (!input.empty()) ProcessCommand(input);
        }
    }

    std::string MiniDBCli::Prompt() {
        return "minidb> ";
    }

    void MiniDBCli::ProcessCommand(const std::string_view input) {
        if (input.find("createtable") == 0) {
            std::cout << "[TODO] Handle CREATE TABLE\n";
        } else if (input.find("insert") == 0) {
            std::cout << "[TODO] Handle INSERT\n";
        } else if (input.find("select") == 0) {
        } else {
            std::cout << "Unrecognized command.\n";
        }
    }

}