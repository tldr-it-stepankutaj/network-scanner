#include "../include/device_identifier.hpp"
#include "../include/tcp.hpp"
#include "../include/icmp.hpp"
#include "../include/logger.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

DeviceIdentifier::DeviceIdentifier() {
    portToService[21] = "FTP";
    portToService[22] = "SSH";
    portToService[23] = "Telnet";
    portToService[25] = "SMTP";
    portToService[53] = "DNS";
    portToService[80] = "HTTP";
    portToService[443] = "HTTPS";
    portToService[445] = "SMB";
    portToService[3389] = "RDP";
    portToService[8080] = "HTTP-Proxy";
    portToService[5900] = "VNC";
    portToService[139] = "NetBIOS";
    portToService[67] = "DHCP";
    portToService[123] = "NTP";
    portToService[161] = "SNMP";
    portToService[9100] = "Printer";
    portToService[62078] = "Apple Device";
    portToService[5353] = "mDNS";
    portToService[5228] = "Android Device";
    portToService[9000] = "Android ADB";
    portToService[7000] = "AirPlay Device";
    portToService[8009] = "Chromecast";
    portToService[8060] = "Roku Device";
    portToService[1900] = "UPNP Device";
    portToService[1883] = "Smart Home Device";
    portToService[1080] = "Security Camera";

    if (std::ifstream file("/usr/share/nmap/nmap-mac-prefixes"); file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string prefix;
            std::string vendor;
            if (iss >> prefix) {
                std::getline(iss >> std::ws, vendor);
                if (!vendor.empty()) {
                    macToVendor[prefix] = vendor;
                }
            }
        }
        Logger::debug("Loaded " + std::to_string(macToVendor.size()) + " MAC vendor entries");
    } else {
        Logger::debug("MAC vendor database not found at /usr/share/nmap/nmap-mac-prefixes");
    }
}

std::string DeviceIdentifier::resolveHostname(const std::string& ip) {
    struct sockaddr_in sa{};
    char hostname[NI_MAXHOST];
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

    if (getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa), hostname, sizeof(hostname), nullptr, 0, NI_NAMEREQD) == 0) {
        Logger::debug("Resolved " + ip + " -> " + std::string(hostname));
        return {hostname};
    }

    return "";
}

std::string DeviceIdentifier::lookupMacVendor(const std::string& ip) {
    if (macToVendor.empty()) return "";

    // Read ARP table to find MAC for this IP
    std::string mac;

#ifdef __linux__
    std::ifstream arpFile("/proc/net/arp");
    if (arpFile.is_open()) {
        std::string line;
        std::getline(arpFile, line); // skip header
        while (std::getline(arpFile, line)) {
            std::istringstream iss(line);
            std::string arpIp, hwType, flags, hwAddr;
            iss >> arpIp >> hwType >> flags >> hwAddr;
            if (arpIp == ip && hwAddr != "00:00:00:00:00:00") {
                mac = hwAddr;
                break;
            }
        }
    }
#elif defined(__APPLE__)
    // On macOS, use arp command output via fork/exec
    int pipefd[2];
    if (pipe(pipefd) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            int devnull = open("/dev/null", O_RDWR);
            if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }
            execlp("arp", "arp", "-n", ip.c_str(), nullptr);
            _exit(127);
        } else if (pid > 0) {
            close(pipefd[1]);
            char buf[512];
            std::string output;
            ssize_t n;
            while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';
                output += buf;
            }
            close(pipefd[0]);
            int status;
            waitpid(pid, &status, 0);

            // Parse "host (ip) at aa:bb:cc:dd:ee:ff ..."
            auto atPos = output.find(" at ");
            if (atPos != std::string::npos) {
                auto macStart = atPos + 4;
                auto macEnd = output.find(' ', macStart);
                std::string candidate = output.substr(macStart, macEnd - macStart);
                if (candidate.find(':') != std::string::npos && candidate != "(incomplete)") {
                    mac = candidate;
                }
            }
        } else {
            close(pipefd[0]);
            close(pipefd[1]);
        }
    }
#endif

    if (mac.empty()) return "";

    // Extract OUI prefix (first 3 octets) and convert to uppercase hex without colons
    std::string oui;
    for (char c : mac) {
        if (c == ':' || c == '-') continue;
        oui += static_cast<char>(toupper(static_cast<unsigned char>(c)));
        if (oui.size() == 6) break;
    }

    auto it = macToVendor.find(oui);
    if (it != macToVendor.end()) {
        Logger::debug("MAC vendor for " + ip + " (" + mac + "): " + it->second);
        return it->second;
    }

    return "";
}

std::string DeviceIdentifier::checkCommonServices(const std::string& ip) {
    if (Tcp::ping(ip, 62078, true)) return "Apple iPhone/iPad";
    if (Tcp::ping(ip, 5228, true) || Tcp::ping(ip, 9000, true)) return "Android Device";
    if (Tcp::ping(ip, 7000, true) || Tcp::ping(ip, 5353, true)) return "Apple Device";
    if (Tcp::ping(ip, 8009, true)) return "Google Chromecast";
    if (Tcp::ping(ip, 8060, true)) return "Roku Device";

    for (const auto& [port, service] : portToService) {
        if (Tcp::ping(ip, port, true)) return service;
    }

    return "";
}

std::string DeviceIdentifier::identifyByPattern(const std::string& ip) {
    const std::string lastOctet = ip.substr(ip.find_last_of('.') + 1);

    if (lastOctet == "1" || lastOctet == "254") return "Possible Router/Gateway";
    if (lastOctet == "10" || lastOctet == "20" || lastOctet == "50" || lastOctet == "100") return "Possible Network Device";

    return "";
}

bool DeviceIdentifier::verifyHost(const std::string& ip) {
    if (const bool icmpSuccess = Icmp::ping(ip, true); !icmpSuccess) {
        int tcpSuccessCount = 0;

        const std::vector<int> commonPorts = {
            80, 443, 22, 21, 23, 25, 53, 3389, 8080, 445,
            7000, 62078, 5353, 5228, 9000, 8009, 8060, 1900
        };

        for (const int port : commonPorts) {
            if (Tcp::ping(ip, port, true)) {
                tcpSuccessCount++;
                if (tcpSuccessCount >= 2) return true;
            }
        }

        return false;
    }

    return Tcp::ping(ip, 80, true) || Tcp::ping(ip, 443, true) ||
           Tcp::ping(ip, 22, true) || Tcp::ping(ip, 8080, true) ||
           Tcp::ping(ip, 62078, true) || Tcp::ping(ip, 7000, true);
}

std::string DeviceIdentifier::identifyDevice(const std::string& ip) {
    Logger::debug("Identifying device: " + ip);

    // First try to resolve hostname
    std::string hostname = resolveHostname(ip);
    if (!hostname.empty()) return hostname;

    // Try MAC vendor lookup
    std::string vendor = lookupMacVendor(ip);
    if (!vendor.empty()) return "Vendor: " + vendor;

    // Try to identify by specific device ports
    if (Tcp::ping(ip, 62078, true)) return "Apple iPhone/iPad";
    if (Tcp::ping(ip, 5228, true) || Tcp::ping(ip, 9000, true)) return "Android Device";
    if (Tcp::ping(ip, 7000, true) && Tcp::ping(ip, 5353, true)) return "Apple TV";
    if (Tcp::ping(ip, 8009, true)) return "Google Chromecast";
    if (Tcp::ping(ip, 8060, true)) return "Roku Device";

    std::string serviceType = checkCommonServices(ip);
    if (!serviceType.empty()) return serviceType;

    if (!verifyHost(ip)) return "Possible Ghost - Unconfirmed";

    std::string patternType = identifyByPattern(ip);
    if (!patternType.empty()) return patternType;

    return "Unknown Device";
}
