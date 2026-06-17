#include "Server.h"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

Server::Server(const std::string& port) : m_port(port), m_server_fd(-1) {}

Server::~Server() {
    if (m_server_fd != -1) {
        close(m_server_fd);
        std::cout << "Server socket closed." << std::endl;
    }
}

void Server::setupSocket() {
    struct addrinfo hints, *res;


    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    if (getaddrinfo(nullptr, m_port.c_str(), &hints, &res) != 0) {
        std::cerr << "Error: getaddrinfo failed." << std::endl;
        exit(EXIT_FAILURE);
    }

    m_server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_server_fd < 0) {
        std::cerr << "Error: Failed to create socket." << std::endl;
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Warning: Failed to set SO_REUSEADDR." << std::endl;
    }

    if (bind(m_server_fd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Error: bind() failed." << std::endl;
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    if (listen(m_server_fd, 10) < 0) {
        std::cerr << "Error: listen() failed." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Server successfully initialized on port " << m_port << std::endl;
}

void Server::acceptLoop() {
    std::cout << "Entering accept loop..." << std::endl;

    while (true) {
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(m_server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            std::cerr << "Warning: Failed to accept connection." << std::endl;
            continue;
        }


        char ip_str[INET6_ADDRSTRLEN];
        if (client_addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
        } else {
            struct sockaddr_in6* s = (struct sockaddr_in6*)&client_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
        }

        std::cout << "Connection accepted from client IP: " << ip_str << std::endl;

        close(client_fd);
    }
}

void Server::start() {
    setupSocket();
    acceptLoop();
}