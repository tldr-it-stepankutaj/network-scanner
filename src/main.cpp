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
#include "../include/colors.hpp"
#include "../include/logger.hpp"
#include "../include/signal_handler.hpp"
#include "../include/utils.hpp"

const std::string VERSION = NetworkAnalyzer::VERSION_STRING;

// JSON helper: escape a string for JSON output
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
        }
    }
    return out;
}

void printVersion() {
    std::cout << VERSION << std::endl;
    std::cout << "Copyright " << NetworkAnalyzer::COPYRIGHT_YEAR << " TLDR;IT s.r.o." << std::endl;
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
    using namespace Colors;
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

void printCompactNetworkInfo(const NetworkInfo& info, const size_t threadCount, const std::string& mode, const int port, const bool thoroughScan, int timeoutMs) {
    using namespace Colors;
    std::cout << GREEN << "[ SYSTEM INFORMATION ]" << RESET << std::endl;
    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;

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
    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "THREADS" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << threadCount << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "SCAN MODE" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << mode << RESET << std::endl;

    if (mode == "tcp" || mode == "fallback") {
        std::cout << BOLD << std::left << std::setw(labelWidth) << "TCP PORT" << RESET << "| "
                  << YELLOW << std::left << std::setw(valueWidth) << port << RESET << std::endl;
    }

    std::cout << BOLD << std::left << std::setw(labelWidth) << "TIMEOUT" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << (std::to_string(timeoutMs) + " ms") << RESET << std::endl;

    if (thoroughScan) {
        std::cout << BOLD << std::left << std::setw(labelWidth) << "SCAN TYPE" << RESET << "| "
                  << RED << std::left << std::setw(valueWidth) << "Thorough" << RESET << std::endl;
    }

    std::cout << std::endl;
}

void promptScan(const std::string& subnet) {
    using namespace Colors;
    std::cout << GREEN << "[ SCAN ]" << RESET << std::endl;
    std::cout << GREEN << "--- Do you want to scan your local subnet (" << YELLOW << subnet << GREEN << ")? (y/n): " << RESET;
}

void displayScanResults(const std::vector<std::pair<std::string, std::string>>& hosts, const std::string& title) {
    using namespace Colors;
    std::cout << GREEN << "[ " << title << " ]" << RESET << std::endl;
    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(18) << "IP ADDRESS" << RESET << "| "
              << BOLD << std::left << std::setw(30) << "DEVICE TYPE" << RESET << std::endl;

    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;

    for (const auto& [ip, deviceType] : hosts) {
        std::cout << YELLOW << std::left << std::setw(18) << ip << RESET << "| "
                  << BRIGHT_GREEN << std::left << std::setw(30) << deviceType << RESET << std::endl;
    }

    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;
    std::cout << std::endl;
}

void displayScanStats(double durationSec, uint32_t totalScanned, size_t hostsFound, size_t confirmedHosts) {
    using namespace Colors;
    std::cout << GREEN << "[ SCAN STATISTICS ]" << RESET << std::endl;
    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;

    constexpr int labelWidth = 20;
    constexpr int valueWidth = 20;

    std::ostringstream durStr;
    durStr << std::fixed << std::setprecision(2) << durationSec << " s";

    double rate = totalScanned > 0 ? (static_cast<double>(hostsFound) / totalScanned * 100.0) : 0.0;
    std::ostringstream rateStr;
    rateStr << std::fixed << std::setprecision(1) << rate << "%";

    std::cout << BOLD << std::left << std::setw(labelWidth) << "DURATION" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << durStr.str() << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "IPs SCANNED" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << totalScanned << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "HOSTS FOUND" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << hostsFound << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "CONFIRMED HOSTS" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << confirmedHosts << RESET << std::endl;

    std::cout << BOLD << std::left << std::setw(labelWidth) << "DISCOVERY RATE" << RESET << "| "
              << YELLOW << std::left << std::setw(valueWidth) << rateStr.str() << RESET << std::endl;

    std::cout << GREEN << std::string(49, '-') << RESET << std::endl;
    std::cout << std::endl;
}

void outputJson(const NetworkInfo& info, const std::string& mode, int port, size_t threadCount,
                bool thoroughScan, int timeoutMs, const std::string& subnet,
                const std::vector<std::pair<std::string, std::string>>& hosts,
                double durationSec, uint32_t totalScanned) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"network_info\": {\n";
    json << "    \"interface\": \"" << jsonEscape(info.interfaceName) << "\",\n";
    json << "    \"local_ip\": \"" << jsonEscape(info.localIp) << "\",\n";
    json << "    \"subnet_mask\": \"" << jsonEscape(info.subnetMask) << "\",\n";
    json << "    \"gateway_ip\": \"" << jsonEscape(info.gatewayIp) << "\",\n";
    json << "    \"public_ip\": \"" << jsonEscape(info.publicIp) << "\"\n";
    json << "  },\n";
    json << "  \"scan_settings\": {\n";
    json << "    \"mode\": \"" << jsonEscape(mode) << "\",\n";
    json << "    \"port\": " << port << ",\n";
    json << "    \"threads\": " << threadCount << ",\n";
    json << "    \"timeout_ms\": " << timeoutMs << ",\n";
    json << "    \"thorough\": " << (thoroughScan ? "true" : "false") << ",\n";
    json << "    \"subnet\": \"" << jsonEscape(subnet) << "\"\n";
    json << "  },\n";
    json << "  \"statistics\": {\n";
    json << "    \"duration_seconds\": " << std::fixed << std::setprecision(3) << durationSec << ",\n";
    json << "    \"ips_scanned\": " << totalScanned << ",\n";
    json << "    \"hosts_found\": " << hosts.size() << "\n";
    json << "  },\n";
    json << "  \"results\": [\n";
    for (size_t i = 0; i < hosts.size(); ++i) {
        json << "    {\"ip\": \"" << jsonEscape(hosts[i].first)
             << "\", \"device_type\": \"" << jsonEscape(hosts[i].second) << "\"}";
        if (i + 1 < hosts.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n";
    json << "}\n";

    std::cout << json.str();
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --threads N       Number of threads to use (default: CPU cores)" << std::endl;
    std::cout << "  --mode MODE       Scan mode: icmp, tcp, or fallback (default: fallback)" << std::endl;
    std::cout << "  --port PORT       Port for TCP scanning (default: 80)" << std::endl;
    std::cout << "  --timeout MS      Probe timeout in milliseconds (default: 1000)" << std::endl;
    std::cout << "  --thorough        Use thorough scanning (higher accuracy, slower)" << std::endl;
    std::cout << "  --skip-scan       Skip network scanning" << std::endl;
    std::cout << "  --json            Output results as JSON (non-interactive)" << std::endl;
    std::cout << "  --no-color        Disable colored output" << std::endl;
    std::cout << "  --verbose         Show informational messages" << std::endl;
    std::cout << "  --debug           Show debug messages" << std::endl;
    std::cout << "  --help            Display this help message" << std::endl;
    std::cout << "  --version         Display version information" << std::endl;
    std::cout << "  --show-all        Show all hosts, including unconfirmed ones" << std::endl;
    std::cout << "  --no-banner       Disable ASCII art banner" << std::endl;
    std::cout << "  --no-clear        Don't clear the screen at start" << std::endl;
}

// Process scan results with progress bar, returns host pairs
static std::vector<std::pair<std::string, std::string>> processHosts(
    DeviceIdentifier& deviceId,
    const std::vector<std::string>& hosts,
    bool jsonOutput)
{
    std::vector<std::pair<std::string, std::string>> hostInfoPairs;

    std::atomic<size_t> counter = 0;
    const size_t total = hosts.size();
    std::atomic<bool> processComplete = false;

    std::thread progressThread;
    if (!jsonOutput) {
        using namespace Colors;
        progressThread = std::thread([&counter, total, &processComplete]() {
            while (!processComplete && !SignalHandler::isInterrupted()) {
                const size_t done = counter.load();
                constexpr int width = 30;
                const int filled = static_cast<int>((static_cast<double>(done) * static_cast<double>(width)) / static_cast<double>(total));

                std::cerr << "\r" << Colors::GREEN << "[" << Colors::YELLOW;
                for (int i = 0; i < width; ++i) {
                    std::cerr << (i < filled ? '#' : '.');
                }
                std::cerr << Colors::GREEN << "] " << Colors::YELLOW << done << "/" << total << Colors::RESET << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    }

    for (const auto& ip : hosts) {
        if (SignalHandler::isInterrupted()) break;
        std::string deviceType = deviceId.identifyDevice(ip);
        hostInfoPairs.emplace_back(ip, deviceType);
        ++counter;
    }

    processComplete = true;
    if (progressThread.joinable()) {
        progressThread.join();
    }
    if (!jsonOutput) {
        std::cerr << "\r" << std::string(80, ' ') << "\r";
    }

    return hostInfoPairs;
}

int main(int argc, char* argv[]) {
    size_t threadCount = std::thread::hardware_concurrency();
    std::string mode = "fallback";
    int port = 80;
    int timeoutMs = 1000;
    bool skipScan = false;
    bool thoroughScan = false;
    bool showAll = false;
    bool showBanner = true;
    bool clearScr = true;
    bool jsonOutput = false;
    bool noColor = false;
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
        } else if (args[i] == "--timeout" && i + 1 < args.size()) {
            try {
                timeoutMs = std::stoi(args[++i]);
                if (timeoutMs < 100 || timeoutMs > 30000) {
                    std::cout << "Timeout must be 100-30000 ms, using default." << std::endl;
                    timeoutMs = 1000;
                }
            } catch (...) {
                std::cout << "Invalid timeout, using default." << std::endl;
            }
        } else if (args[i] == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (args[i] == "--version" || args[i] == "-v") {
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
        } else if (args[i] == "--json") {
            jsonOutput = true;
        } else if (args[i] == "--no-color") {
            noColor = true;
        } else if (args[i] == "--verbose") {
            Logger::setLevel(Logger::Level::VERBOSE);
        } else if (args[i] == "--debug") {
            Logger::setLevel(Logger::Level::DEBUG);
        }
    }

    // Disable colors if requested or NO_COLOR env is set
    if (noColor || Colors::shouldDisable()) {
        Colors::disable();
    }

    // In JSON mode, suppress interactive UI
    if (jsonOutput) {
        showBanner = false;
        clearScr = false;
        Logger::setLevel(Logger::Level::QUIET);
    }

    // Install signal handler for graceful Ctrl+C
    SignalHandler::install();

    if (clearScr) {
        clearScreen();
    }

    if (showBanner) {
        printPixelAsciiArt();
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    NetworkInfo info = getNetworkInfo();

    if (!jsonOutput) {
        printCompactNetworkInfo(info, threadCount, mode, port, thoroughScan, timeoutMs);
    }

    // Use actual subnet mask instead of hardcoded /24
    std::string localSubnet;
    if (!info.subnetMask.empty() && !info.localIp.empty()) {
        localSubnet = ipToCIDR(info.localIp, info.subnetMask);
    } else {
        localSubnet = getSubnet24(info.localIp);
    }

    std::string gatewaySubnet;
    if (!info.gatewayIp.empty()) {
        if (!info.subnetMask.empty()) {
            gatewaySubnet = ipToCIDR(info.gatewayIp, info.subnetMask);
        } else {
            gatewaySubnet = getSubnet24(info.gatewayIp);
        }
    }

    bool differentSubnets = (localSubnet != gatewaySubnet) && !info.gatewayIp.empty();

    // Warn if subnet is larger than /16
    {
        auto [start, end] = Utils::parseCIDR(localSubnet);
        uint32_t hostCount = end - start + 1;
        if (hostCount > 65536 && !jsonOutput) {
            using namespace Colors;
            std::cout << RED << "[!] WARNING: Subnet " << localSubnet << " contains " << hostCount
                      << " hosts. This scan may take a very long time." << RESET << std::endl;
            std::cout << RED << "[!] Consider using a smaller subnet range." << RESET << std::endl;
        }
    }

    if (skipScan) {
        if (!jsonOutput) {
            using namespace Colors;
            std::cout << GREEN << "[!] Skipping network scan as requested." << RESET << std::endl;
        }
        curl_global_cleanup();
        return returnCode;
    }

    // In JSON mode, skip the prompt and auto-scan
    if (!jsonOutput) {
        promptScan(localSubnet);
        char response;
        std::cin >> response;
        if (tolower(response) != 'y') {
            curl_global_cleanup();
            return returnCode;
        }
    }

    {
        using namespace Colors;
        DeviceIdentifier deviceId;

        if (!jsonOutput) {
            std::cout << std::endl << GREEN << "[+] Starting scan of " << YELLOW << localSubnet << RESET << std::endl;
        }

        NetworkScanner scanner(threadCount, mode, port, timeoutMs);

        auto scanStart = std::chrono::steady_clock::now();

        std::vector<std::string> localHosts;
        try {
            if (thoroughScan) {
                if (!jsonOutput) {
                    std::cout << GREEN << "[+] Using thorough scan mode (may take longer)" << RESET << std::endl;
                }
                localHosts = scanner.thoroughScan(localSubnet);
            } else {
                localHosts = scanner.scan(localSubnet);
            }
        } catch (const std::exception& e) {
            std::cerr << RED << "[!] Error during scan: " << e.what() << RESET << std::endl;
            curl_global_cleanup();
            return 1;
        }

        if (SignalHandler::isInterrupted()) {
            if (!jsonOutput) {
                std::cout << std::endl << YELLOW << "[!] Scan interrupted by user. Showing partial results." << RESET << std::endl;
            }
        }

        if (!jsonOutput) {
            std::cout << GREEN << "[+] Processing results..." << RESET << std::endl;
        }

        auto hostInfoPairs = processHosts(deviceId, localHosts, jsonOutput);

        auto scanEnd = std::chrono::steady_clock::now();
        double durationSec = std::chrono::duration<double>(scanEnd - scanStart).count();

        // Calculate total IPs scanned
        auto [start, end] = Utils::parseCIDR(localSubnet);
        uint32_t totalScanned = end - start + 1;

        std::vector<std::pair<std::string, std::string>> confirmedHostInfoPairs;
        for (const auto& [ip, deviceType] : hostInfoPairs) {
            if (deviceType != "Possible Ghost - Unconfirmed") {
                confirmedHostInfoPairs.emplace_back(ip, deviceType);
            }
        }

        if (jsonOutput) {
            auto& displayPairs = showAll ? hostInfoPairs : confirmedHostInfoPairs;
            outputJson(info, mode, port, threadCount, thoroughScan, timeoutMs,
                      localSubnet, displayPairs, durationSec, totalScanned);
        } else {
            auto& displayPairs = showAll ? hostInfoPairs : confirmedHostInfoPairs;

            if (showAll) {
                std::cout << GREEN << "[+] Found " << YELLOW << hostInfoPairs.size() << GREEN << " live hosts on local subnet" << RESET << std::endl;
            } else {
                std::cout << GREEN << "[+] Found " << YELLOW << confirmedHostInfoPairs.size() << GREEN << " confirmed live hosts out of "
                          << YELLOW << hostInfoPairs.size() << GREEN << " total hosts detected" << RESET << std::endl;
            }

            displayScanResults(displayPairs, "SCAN RESULTS");
            displayScanStats(durationSec, totalScanned, hostInfoPairs.size(), confirmedHostInfoPairs.size());

            if (differentSubnets && !SignalHandler::isInterrupted()) {
                std::cout << GREEN << "[ GATEWAY ]" << RESET << std::endl;
                std::cout << GREEN << "--- Your gateway (" << YELLOW << info.gatewayIp << GREEN
                          << ") is in a different subnet (" << YELLOW << gatewaySubnet << GREEN << ")" << RESET << std::endl;
                std::cout << GREEN << "--- Do you want to scan the gateway subnet as well? (y/n): " << RESET;

                char response;
                std::cin >> response;
                if (tolower(response) == 'y') {
                    std::cout << std::endl << GREEN << "[+] Starting scan of " << YELLOW << gatewaySubnet << RESET << std::endl;

                    auto gwScanStart = std::chrono::steady_clock::now();

                    std::vector<std::string> gatewayHosts;
                    try {
                        if (thoroughScan) {
                            gatewayHosts = scanner.thoroughScan(gatewaySubnet);
                        } else {
                            gatewayHosts = scanner.scan(gatewaySubnet);
                        }
                    } catch (const std::exception& e) {
                        std::cerr << RED << "[!] Error during gateway scan: " << e.what() << RESET << std::endl;
                        curl_global_cleanup();
                        return 1;
                    }

                    std::cout << GREEN << "[+] Processing results..." << RESET << std::endl;

                    auto gatewayHostInfoPairs = processHosts(deviceId, gatewayHosts, jsonOutput);

                    auto gwScanEnd = std::chrono::steady_clock::now();
                    double gwDuration = std::chrono::duration<double>(gwScanEnd - gwScanStart).count();

                    auto [gwStart, gwEnd] = Utils::parseCIDR(gatewaySubnet);
                    uint32_t gwTotalScanned = gwEnd - gwStart + 1;

                    std::vector<std::pair<std::string, std::string>> confirmedGatewayHostInfoPairs;
                    for (const auto& [ip, deviceType] : gatewayHostInfoPairs) {
                        if (deviceType != "Possible Ghost - Unconfirmed") {
                            confirmedGatewayHostInfoPairs.emplace_back(ip, deviceType);
                        }
                    }

                    auto& displayGatewayPairs = showAll ? gatewayHostInfoPairs : confirmedGatewayHostInfoPairs;

                    if (showAll) {
                        std::cout << GREEN << "[+] Found " << YELLOW << gatewayHostInfoPairs.size() << GREEN << " live hosts on gateway subnet" << RESET << std::endl;
                    } else {
                        std::cout << GREEN << "[+] Found " << YELLOW << confirmedGatewayHostInfoPairs.size() << GREEN << " confirmed live hosts out of "
                                  << YELLOW << gatewayHostInfoPairs.size() << GREEN << " total hosts detected on gateway subnet" << RESET << std::endl;
                    }

                    displayScanResults(displayGatewayPairs, "GATEWAY RESULTS");
                    displayScanStats(gwDuration, gwTotalScanned, gatewayHostInfoPairs.size(), confirmedGatewayHostInfoPairs.size());
                }
            }
        }
    }

    curl_global_cleanup();

    return returnCode;
}
