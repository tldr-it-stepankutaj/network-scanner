#include "../include/device_identifier.hpp"
#include "../include/tcp.hpp"
#include "../include/icmp.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>

DeviceIdentifier::DeviceIdentifier() {
    // Initialize common port to service mapping
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
    portToService[62078] = "Apple Device"; // iPhone sync
    portToService[5353] = "mDNS";          // Often used by Apple devices
    portToService[5228] = "Android Device"; // Google Play services
    portToService[9000] = "Android ADB";    // Android Debug Bridge

    // Add more device-specific ports
    portToService[7000] = "AirPlay Device"; // AirPlay protocol
    portToService[8009] = "Chromecast";     // Chromecast device
    portToService[8060] = "Roku Device";    // Roku media player
    portToService[1900] = "UPNP Device";    // Universal Plug and Play
    portToService[1883] = "Smart Home Device"; // MQTT protocol often used by IoT devices
    portToService[1080] = "Security Camera"; // Common for security cameras

    // Load MAC vendor database if available
    if (std::ifstream file("/usr/share/nmap/nmap-mac-prefixes"); file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string vendor;
            if (std::string prefix; iss >> prefix >> vendor) {
                macToVendor[prefix] = vendor;
            }
        }
    }
}

std::string DeviceIdentifier::resolveHostname(const std::string& ip) {
    struct sockaddr_in sa{};
    char hostname[NI_MAXHOST];
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

    if (getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa), hostname, sizeof(hostname), nullptr, 0, NI_NAMEREQD) == 0) {
        return {hostname};
    }

    return "";
}

std::string DeviceIdentifier::checkCommonServices(const std::string& ip) {
    // Check for iPhone sync port specifically
    if (Tcp::ping(ip, 62078, true)) {
        return "Apple iPhone/iPad";
    }

    // Check for Android specific ports
    if (Tcp::ping(ip, 5228, true) || Tcp::ping(ip, 9000, true)) {
        return "Android Device";
    }

    // Check for AirPlay/Apple TV
    if (Tcp::ping(ip, 7000, true) || Tcp::ping(ip, 5353, true)) {
        return "Apple Device";
    }

    // Check for Smart TV/Streaming devices
    if (Tcp::ping(ip, 8009, true)) {
        return "Google Chromecast";
    }

    if (Tcp::ping(ip, 8060, true)) {
        return "Roku Device";
    }

    // For other services, check our full mapping
    for (const auto& [port, service] : portToService) {
        if (Tcp::ping(ip, port, true)) {
            return service;
        }
    }

    return "";
}

std::string DeviceIdentifier::identifyByPattern(const std::string& ip) {
    // Extract the last octet of the IP address
    const std::string lastOctet = ip.substr(ip.find_last_of('.') + 1);

    // Common gateway addresses - be more selective now
    if (lastOctet == "1" || lastOctet == "254") {
        return "Possible Router/Gateway";
    }

    // Common patterns for network devices
    if (lastOctet == "10" || lastOctet == "20" || lastOctet == "50" || lastOctet == "100") {
        return "Possible Network Device";
    }

    return "";
}

bool DeviceIdentifier::verifyHost(const std::string& ip) {
    // Try ICMP ping first - most reliable method

    if (const bool icmpSuccess = Icmp::ping(ip, true); !icmpSuccess) {
        // If ICMP fails, try multiple TCP ports that are commonly open
        int tcpSuccessCount = 0;

        // Check common ports (similar to what Nmap checks)
        const std::vector<int> commonPorts = {
            80, 443, 22, 21, 23, 25, 53, 3389, 8080, 445,
            7000, 62078, 5353, 5228, 9000, 8009, 8060, 1900
        };

        for (const int port : commonPorts) {
            if (Tcp::ping(ip, port, true)) {
                tcpSuccessCount++;
                if (tcpSuccessCount >= 2) {
                    return true;  // Found at least 2 open ports, consider it alive
                }
            }
        }

        // If we get here, we didn't find enough open TCP ports
        return false;
    }

    // For hosts that respond to ICMP, check at least one additional confirmation
    // Try some common ports to see if any respond
    return Tcp::ping(ip, 80, true) || Tcp::ping(ip, 443, true) ||
           Tcp::ping(ip, 22, true) || Tcp::ping(ip, 8080, true) ||
           Tcp::ping(ip, 62078, true) || Tcp::ping(ip, 7000, true);
}

std::string DeviceIdentifier::identifyDevice(const std::string& ip) {
    // First try to resolve hostname (most accurate)
    std::string hostname = resolveHostname(ip);
    if (!hostname.empty()) {
        return hostname;
    }

    // Try to identify by specific device ports
    // iPhone/iPad detection
    if (Tcp::ping(ip, 62078, true)) {
        return "Apple iPhone/iPad";
    }

    // Android detection
    if (Tcp::ping(ip, 5228, true) || Tcp::ping(ip, 9000, true)) {
        return "Android Device";
    }

    // AirPlay/Apple TV detection
    if (Tcp::ping(ip, 7000, true) && Tcp::ping(ip, 5353, true)) {
        return "Apple TV";
    }

    // Smart TV/Streaming device detection
    if (Tcp::ping(ip, 8009, true)) {
        return "Google Chromecast";
    }

    if (Tcp::ping(ip, 8060, true)) {
        return "Roku Device";
    }

    // Then try to identify by service detection for other devices
    std::string serviceType = checkCommonServices(ip);
    if (!serviceType.empty()) {
        return serviceType;
    }

    // Try to verify if it's truly a live host with double-check
    if (!verifyHost(ip)) {
        return "Possible Ghost - Unconfirmed";
    }

    // Last resort: pattern matching
    std::string patternType = identifyByPattern(ip);
    if (!patternType.empty()) {
        return patternType;
    }

    // Default fallback
    return "Unknown Device";
}