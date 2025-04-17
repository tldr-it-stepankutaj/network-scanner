#pragma once
#include <string>
#include <cstddef>

class Scanner {
public:
    explicit Scanner(size_t threads = 0, std::string mode = "icmp", int port = 80);
    void run(const std::string& cidr) const;

private:
    size_t threadCount;
    std::string mode;
    int port;
};