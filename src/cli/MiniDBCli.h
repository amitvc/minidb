//
// Created by Amit Chavan on 6/6/25.
//

#pragma once
#include <string>

namespace minidb {
    class MiniDBCli {
    public:
        void Run();

    private:
        void ProcessCommand(const std::string &input);

        std::string Prompt();

    };
}


