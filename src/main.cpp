#include "Server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Default to port 8080 if none is specified via command line arguments
    std::string port = "8080";
    if (argc >= 2) {
        port = argv[1];
    }

    Server server(port);
    server.start();

    return 0;
}