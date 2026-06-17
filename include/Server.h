//
// Created by moham on 17/06/2026.
//

#ifndef SERVER_H
#define SERVER_H
#include <string>
using namespace std;

class Server {
public:
    Server(const string& port);
    ~Server() = default;
    void start();
private:
    std::string m_port;
    int m_server_fd;
    void setupSocket();
    void acceptLoop();
};
#endif //SERVER_H
