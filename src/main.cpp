//
// Main entry point for MiniDB CLI application
//

#include "cli/MiniDBCli.h"

int main() {
    minidb::MiniDBCli cli;
    cli.Run();
    return 0;
}