#include "../include/tcp.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>
#include <iostream>
#include <mutex>

namespace Tcp {
    // Protect console output
    static std::mutex outputMutex;

    bool ping(const std::string& ip, int port, bool quiet) {
        const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) return false;

        fcntl(sockfd, F_SETFL, O_NONBLOCK);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        const auto start = std::chrono::steady_clock::now();

        if (const int connResult = connect(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            connResult < 0 && errno != EINPROGRESS) {
            close(sockfd);
            return false; // connection failed immediately
        }

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

                if (!quiet) {
                    std::lock_guard<std::mutex> lock(outputMutex);

                    std::string color = "\033[0m";
                    if (ms < 30)       color = "\033[32m";
                    else if (ms < 100) color = "\033[33m";
                    else               color = "\033[31m";

                    std::cerr << color << ip << " is alive via TCP port " << port
                              << " (RTT: " << ms << " ms)\033[0m" << std::endl;
                }

                success = true;
            }
        }

        close(sockfd);
        return success;
    }
}