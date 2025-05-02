#pragma once
#include <string>
#include <cstddef>
#include <vector>
#include <algorithm>

class Scanner {
public:
    explicit Scanner(size_t threads = 0, std::string mode = "icmp", int port = 80);
    virtual void run(const std::string& cidr) const;
    virtual ~Scanner() = default;

protected:
    size_t threadCount;
    std::string mode;
    int port;
};

class NetworkScanner final : public Scanner {
public:
    explicit NetworkScanner(size_t threads = 0, std::string mode = "icmp", int port = 80);
    [[nodiscard]] std::vector<std::string> scan(const std::string& cidr) const;
    [[nodiscard]] std::vector<std::string> thoroughScan(const std::string& cidr) const;
    ~NetworkScanner() override = default;

private:
    // Single declaration with the nodiscard attribute
    [[nodiscard]] static bool verifyHost(const std::string& ip);
};