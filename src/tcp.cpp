#include "tcp.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>
#include <iostream>

namespace Tcp {

    bool ping(const std::string& ip, int port) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) return false;

        fcntl(sockfd, F_SETFL, O_NONBLOCK);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        auto start = std::chrono::steady_clock::now();

        connect(sockfd, (sockaddr*)&addr, sizeof(addr));

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        timeval timeout{1, 0};

        bool success = false;

        if (select(sockfd + 1, nullptr, &fds, nullptr, &timeout) > 0) {
            int so_error = -1;
            socklen_t len = sizeof(so_error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error == 0) {
                auto end = std::chrono::steady_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

                std::string color = "\033[0m";
                if (ms < 30)       color = "\033[32m";
                else if (ms < 100) color = "\033[33m";
                else               color = "\033[31m";

                std::cout << color << ip << " is alive via TCP port " << port
                          << " (RTT: " << ms << " ms)\033[0m\n";

                success = true;
            }
        }

        close(sockfd);
        return success;
    }
}