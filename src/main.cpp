#include <iostream>
#include <string>
#include <thread>
#include "scanner.hpp"

const std::string VERSION = "network-scanner v1.0.0";

void print_help(const std::string& name) {
    std::cout << VERSION << "\n"
              << "Usage:\n"
              << "  " << name << " <CIDR> [options]\n\n"
              << "Options:\n"
              << "  --threads N          Number of parallel threads (default: CPU cores)\n"
              << "  --mode MODE          icmp | tcp | fallback (default: icmp)\n"
              << "  --port PORT          Port for TCP or fallback mode (default: 80)\n"
              << "  --help               Show this help message\n"
              << "  --version            Show version info\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--help") {
        print_help(argv[0]);
        return 0;
    }
    if (arg1 == "--version") {
        std::cout << VERSION << std::endl;
        return 0;
    }

    std::string cidr = argv[1];
    size_t threads = std::thread::hardware_concurrency();
    std::string mode = "icmp";
    int port = 80;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "--version") {
            continue; // handled above
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_help(argv[0]);
            return 1;
        }
    }

    Scanner scanner(threads, mode, port);
    scanner.run(cidr);
    return 0;
}