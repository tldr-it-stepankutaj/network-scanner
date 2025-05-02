#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <algorithm>
#include <curl/curl.h>
#include <vector>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include "../include/version.hpp"

#include "../include/scanner.hpp"
#include "../include/network_info.hpp"
#include "../include/device_identifier.hpp"

const std::string VERSION = NetworkAnalyzer::VERSION_STRING;
const std::string GREEN = "\033[32m";
const std::string BRIGHT_GREEN = "\033[92m";
const std::string CYAN = "\033[36m";
const std::string YELLOW = "\033[33m";
const std::string RED = "\033[31m";
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";

void printVersion() {
    std::cout << VERSION << std::endl;
    std::cout << "Copyright © " << NetworkAnalyzer::COPYRIGHT_YEAR << " TLDR;IT s.r.o." << std::endl;
    std::cout << "License: MIT" << std::endl;
    std::cout << "Built with: C++ " << __cplusplus << std::endl;
}

void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void printPixelAsciiArt() {
    std::cout << GREEN << R"(
███╗   ██╗███████╗████████╗██╗    ██╗ ██████╗ ██████╗ ██╗  ██╗
████╗  ██║██╔════╝╚══██╔══╝██║    ██║██╔═══██╗██╔══██╗██║ ██╔╝
██╔██╗ ██║█████╗     ██║   ██║ █╗ ██║██║   ██║██████╔╝█████╔╝
██║╚██╗██║██╔══╝     ██║   ██║███╗██║██║   ██║██╔══██╗██╔═██╗
██║ ╚████║███████╗   ██║   ╚███╔███╔╝╚██████╔╝██║  ██║██║  ██╗
╚═╝  ╚═══╝╚══════╝   ╚═╝    ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝

 █████╗ ███╗   ██╗ █████╗ ██╗  ██╗   ██╗███████╗███████╗██████╗
██╔══██╗████╗  ██║██╔══██╗██║  ╚██╗ ██╔╝██╔════╝██╔════╝██╔══██╗
███████║██╔██╗ ██║███████║██║   ╚████╔╝ ███████╗█████╗  ██████╔╝
██╔══██║██║╚██╗██║██╔══██║██║    ╚██╔╝  ╚════██║██╔══╝  ██╔══██╗
██║  ██║██║ ╚████║██║  ██║███████╗██║   ███████║███████╗██║  ██║
╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝╚═╝   ╚══════╝╚══════╝╚═╝  ╚═╝
)" << RESET << std::endl;

    std::cout << GREEN << "[ Initializing scan modules... ]" << RESET << std::endl;
    std::cout << GREEN << "[ Scanning network interface... ]" << RESET << std::endl;

    std::mt19937 gen(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<> dis(0, 1);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 50; j++) {
            std::cout << GREEN << dis(gen);
        }
        std::cout << RESET << std::endl;
    }

    std::cout << GREEN << "[ Systems online. Ready to scan. ]" << RESET << std::endl << std::endl;
}

void printCompactNetworkInfo(const NetworkInfo& info, const size_t threadCount, const std::string& mode, const int port, const bool thoroughScan) {
    std::cout << GREEN << "[ SYSTEM INFORMATION ]" << RESET << std::endl;
    std::cout << GREEN << "─────────────────────────────────────────────────" << RESET << std::endl;

    constexpr int valueWidth = 20;

    constexpr int labelWidth = 20;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "NETWORK INTERFACE" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << info.interfaceName << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "LOCAL IP ADDRESS" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << info.localIp << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "SUBNET MASK" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << info.subnetMask << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "GATEWAY IP ADDRESS" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << (info.gatewayIp.empty() ? "Not detected" : info.gatewayIp) << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "PUBLIC IP ADDRESS" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << info.publicIp << RESET << std::endl;

    std::cout << std::endl << GREEN << "[ SCANNER SETTINGS ]" << RESET << std::endl;
    std::cout << GREEN << "─────────────────────────────────────────────────" << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "THREADS" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << threadCount << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "SCAN MODE" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << mode << RESET << std::endl;

    if (mode == "tcp" || mode == "fallback") {
        std::cout << BOLD << std::left << std::setw(labelWidth) << "TCP PORT" << RESET << "| "
                  << YELLOW << std::left << std::setw(valueWidth) << port << RESET << std::endl;
    }

    if (thoroughScan) {
        std::cout << BOLD << std::left << std::setw(labelWidth) << "SCAN TYPE" << RESET << "| "
                  << RED << std::left << std::setw(valueWidth) << "Thorough" << RESET << std::endl;
    }

    std::cout << std::endl;
}

void promptScan(const std::string& subnet) {
    std::cout << GREEN << "[ SCAN ]" << RESET << std::endl;
    std::cout << GREEN << "─── Do you want to scan your local subnet (" << YELLOW << subnet << GREEN << ")? (y/n): " << RESET;
}

void displayScanResults(const std::vector<std::pair<std::string, std::string>>& hosts, const std::string& title) {
    std::cout << GREEN << "[ " << title << " ]" << RESET << std::endl;
    std::cout << GREEN << "─────────────────────────────────────────────────" << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(18) << "IP ADDRESS" << RESET << "| "
              << BOLD << std::left << std::setw(30) << "DEVICE TYPE" << RESET << std::endl;

    std::cout << GREEN << "─────────────────────────────────────────────────" << RESET << std::endl;

    for (const auto& [ip, deviceType] : hosts) {
        std::cout << YELLOW << std::left << std::setw(18) << ip << RESET << "| "
                  << BRIGHT_GREEN << std::left << std::setw(30) << deviceType << RESET << std::endl;
    }

    std::cout << GREEN << "─────────────────────────────────────────────────" << RESET << std::endl;
    std::cout << std::endl;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --threads N       Number of threads to use (default: CPU cores)" << std::endl;
    std::cout << "  --mode MODE       Scan mode: icmp, tcp, or fallback (default: fallback)" << std::endl;
    std::cout << "  --port PORT       Port for TCP scanning (default: 80)" << std::endl;
    std::cout << "  --thorough        Use thorough scanning (higher accuracy, slower)" << std::endl;
    std::cout << "  --skip-scan       Skip network scanning" << std::endl;
    std::cout << "  --help            Display this help message" << std::endl;
    std::cout << "  --version         Display version information" << std::endl;
    std::cout << "  --show-all        Show all hosts, including unconfirmed ones" << std::endl;
    std::cout << "  --no-banner       Disable ASCII art banner" << std::endl;
    std::cout << "  --no-clear        Don't clear the screen at start" << std::endl;
}

int main(int argc, char* argv[]) {
    size_t threadCount = std::thread::hardware_concurrency();
    std::string mode = "fallback";
    int port = 80;
    bool skipScan = false;
    bool thoroughScan = false;
    bool showAll = false;
    bool showBanner = true;
    bool clearScr = true;
    int returnCode = 0;

    std::vector<std::string> args(argv, argv + argc);
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--threads" && i + 1 < args.size()) {
            try {
                threadCount = std::stoi(args[++i]);
                if (threadCount < 1) {
                    std::cout << "Invalid thread count, using default." << std::endl;
                    threadCount = std::thread::hardware_concurrency();
                }
            } catch (...) {
                std::cout << "Invalid thread count, using default." << std::endl;
            }
        } else if (args[i] == "--mode" && i + 1 < args.size()) {
            mode = args[++i];
            if (mode != "icmp" && mode != "tcp" && mode != "fallback") {
                std::cout << "Invalid mode, using default." << std::endl;
                mode = "fallback";
            }
        } else if (args[i] == "--port" && i + 1 < args.size()) {
            try {
                port = std::stoi(args[++i]);
                if (port < 1 || port > 65535) {
                    std::cout << "Invalid port, using default." << std::endl;
                    port = 80;
                }
            } catch (...) {
                std::cout << "Invalid port, using default." << std::endl;
            }
        } else if (args[i] == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (args[i] == "--version" || args[i] == "-v") {
            // Přidána podpora pro --version a -v
            printVersion();
            return 0;
        } else if (args[i] == "--skip-scan") {
            skipScan = true;
        } else if (args[i] == "--thorough") {
            thoroughScan = true;
        } else if (args[i] == "--show-all") {
            showAll = true;
        } else if (args[i] == "--no-banner") {
            showBanner = false;
        } else if (args[i] == "--no-clear") {
            clearScr = false;
        }
    }

    if (clearScr) {
        clearScreen();
    }

    if (showBanner) {
        printPixelAsciiArt();
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    NetworkInfo info = getNetworkInfo();

    printCompactNetworkInfo(info, threadCount, mode, port, thoroughScan);

    std::string localSubnet = getSubnet24(info.localIp);
    std::string gatewaySubnet = getSubnet24(info.gatewayIp);

    bool differentSubnets = (localSubnet != gatewaySubnet) && !info.gatewayIp.empty();

    if (skipScan) {
        std::cout << GREEN << "[!] Skipping network scan as requested." << RESET << std::endl;
        curl_global_cleanup();
        return returnCode;
    }

    promptScan(localSubnet);
    char response;
    std::cin >> response;

    if (tolower(response) == 'y') {
        DeviceIdentifier deviceId;

        std::cout << std::endl << GREEN << "[+] Starting scan of " << YELLOW << localSubnet << RESET << std::endl;

        NetworkScanner scanner(threadCount, mode, port);

        std::vector<std::string> localHosts;
        try {
            if (thoroughScan) {
                std::cout << GREEN << "[+] Using thorough scan mode (may take longer)" << RESET << std::endl;
                localHosts = scanner.thoroughScan(localSubnet);
            } else {
                localHosts = scanner.scan(localSubnet);
            }
        } catch (const std::exception& e) {
            std::cerr << RED << "[!] Error during scan: " << e.what() << RESET << std::endl;
            returnCode = 1;
            curl_global_cleanup();
            return returnCode;
        }

        std::vector<std::pair<std::string, std::string>> hostInfoPairs;
        std::vector<std::pair<std::string, std::string>> confirmedHostInfoPairs;

        std::cout << GREEN << "[+] Processing results..." << RESET << std::endl;

        std::atomic<size_t> counter = 0;
        const size_t total = localHosts.size();
        std::atomic<bool> processComplete = false;
        std::thread progressThread([&counter, total, &processComplete]() {
            while (!processComplete) {
                const size_t done = counter.load();
                constexpr int width = 30;
                const int filled = static_cast<int>((static_cast<double>(done) * static_cast<double>(width)) / static_cast<double>(total));

                std::cerr << "\r" << GREEN << "[" << YELLOW;
                for (int i = 0; i < width; ++i) {
                    std::cerr << (i < filled ? '#' : '.');
                }
                std::cerr << GREEN << "] " << YELLOW << done << "/" << total << RESET << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        for (const auto& ip : localHosts) {
            std::string deviceType = deviceId.identifyDevice(ip);
            hostInfoPairs.emplace_back(ip, deviceType);

            if (deviceType != "Possible Ghost - Unconfirmed") {
                confirmedHostInfoPairs.emplace_back(ip, deviceType);
            }
            ++counter;
        }

        processComplete = true;
        progressThread.join();
        std::cerr << "\r" << std::string(80, ' ') << "\r";

        std::vector<std::pair<std::string, std::string>>& displayPairs = showAll ? hostInfoPairs : confirmedHostInfoPairs;

        if (showAll) {
            std::cout << GREEN << "[+] Found " << YELLOW << hostInfoPairs.size() << GREEN << " live hosts on local subnet" << RESET << std::endl;
        } else {
            std::cout << GREEN << "[+] Found " << YELLOW << confirmedHostInfoPairs.size() << GREEN << " confirmed live hosts out of "
                      << YELLOW << hostInfoPairs.size() << GREEN << " total hosts detected" << RESET << std::endl;
        }

        displayScanResults(displayPairs, "SCAN RESULTS");

        if (differentSubnets) {
            std::cout << GREEN << "[ GATEWAY ]" << RESET << std::endl;
            std::cout << GREEN << "─── Your gateway (" << YELLOW << info.gatewayIp << GREEN
                      << ") is in a different subnet (" << YELLOW << gatewaySubnet << GREEN << ")" << RESET << std::endl;
            std::cout << GREEN << "─── Do you want to scan the gateway subnet as well? (y/n): " << RESET;

            std::cin >> response;
            if (tolower(response) == 'y') {
                std::cout << std::endl << GREEN << "[+] Starting scan of " << YELLOW << gatewaySubnet << RESET << std::endl;

                std::vector<std::string> gatewayHosts;
                try {
                    if (thoroughScan) {
                        gatewayHosts = scanner.thoroughScan(gatewaySubnet);
                    } else {
                        gatewayHosts = scanner.scan(gatewaySubnet);
                    }
                } catch (const std::exception& e) {
                    std::cerr << RED << "[!] Error during gateway scan: " << e.what() << RESET << std::endl;
                    returnCode = 1;
                    curl_global_cleanup();
                    return returnCode;
                }

                std::vector<std::pair<std::string, std::string>> gatewayHostInfoPairs;
                std::vector<std::pair<std::string, std::string>> confirmedGatewayHostInfoPairs;

                std::cout << GREEN << "[+] Processing results..." << RESET << std::endl;

                counter = 0;
                processComplete = false;

                std::thread gatewayProgressThread([&counter, &gatewayHosts, &processComplete]() {
                    const size_t gwTotal = gatewayHosts.size();
                    while (!processComplete) {
                        const size_t done = counter.load();
                        constexpr int width = 30;
                        const int filled = static_cast<int>((static_cast<double>(done) * static_cast<double>(width)) / static_cast<double>(gwTotal));

                        std::cerr << "\r" << GREEN << "[" << YELLOW;
                        for (int i = 0; i < width; ++i) {
                            std::cerr << (i < filled ? '#' : '.');
                        }
                        std::cerr << GREEN << "] " << YELLOW << done << "/" << gwTotal << RESET << std::flush;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                });

                for (const auto& ip : gatewayHosts) {
                    std::string deviceType = deviceId.identifyDevice(ip);
                    gatewayHostInfoPairs.emplace_back(ip, deviceType);

                    if (deviceType != "Possible Ghost - Unconfirmed") {
                        confirmedGatewayHostInfoPairs.emplace_back(ip, deviceType);
                    }
                    ++counter;
                }

                processComplete = true;
                gatewayProgressThread.join();
                std::cerr << "\r" << std::string(80, ' ') << "\r";

                std::vector<std::pair<std::string, std::string>>& displayGatewayPairs =
                    showAll ? gatewayHostInfoPairs : confirmedGatewayHostInfoPairs;

                if (showAll) {
                    std::cout << GREEN << "[+] Found " << YELLOW << gatewayHostInfoPairs.size() << GREEN << " live hosts on gateway subnet" << RESET << std::endl;
                } else {
                    std::cout << GREEN << "[+] Found " << YELLOW << confirmedGatewayHostInfoPairs.size() << GREEN << " confirmed live hosts out of "
                              << YELLOW << gatewayHostInfoPairs.size() << GREEN << " total hosts detected on gateway subnet" << RESET << std::endl;
                }

                displayScanResults(displayGatewayPairs, "GATEWAY RESULTS");
            }
        }
    }

    curl_global_cleanup();

    return returnCode;
}