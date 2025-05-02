#include "../include/network_info.hpp"
#include "../include/utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <curl/curl.h>

// Function to write callback for curl
size_t WriteCallback(void* contents, const size_t size, const size_t nmemb, std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

std::string getPublicIP() {
    std::string readBuffer;

    if (CURL* curl = curl_easy_init()) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.ipify.org");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        const CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return "Error: " + std::string(curl_easy_strerror(res));
        }
    }

    return readBuffer;
}

NetworkInfo getNetworkInfo() {
    NetworkInfo info;

    // Get local IP and interface name
    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) == 0) {
        for (struct ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                // Skip loopback interface
                if (strcmp(ifa->ifa_name, "lo") == 0) continue;

                // Get IP address
                auto* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                char buffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa->sin_addr), buffer, INET_ADDRSTRLEN);

                // Skip IPv6 and non-routable addresses
                std::string ip(buffer);
                if (ip.find("127.") == 0 || ip.find("169.254.") == 0) continue;

                info.localIp = ip;
                info.interfaceName = ifa->ifa_name;

                // Get subnet mask
                if (ifa->ifa_netmask) {
                    auto* mask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);
                    char maskBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(mask->sin_addr), maskBuffer, INET_ADDRSTRLEN);
                    info.subnetMask = maskBuffer;
                }

                break;
            }
        }
        freeifaddrs(ifap);
    }

    // Get gateway IP by reading /proc/net/route
    if (std::ifstream routeFile("/proc/net/route"); routeFile.is_open()) {
        std::string line;
        // Skip header line
        std::getline(routeFile, line);

        while (std::getline(routeFile, line)) {
            std::istringstream iss(line);
            std::string iface, dest, gateway;
            iss >> iface >> dest >> gateway;

            // Default route has destination 00000000
            if (dest == "00000000" && iface == info.interfaceName) {
                // Convert hex gateway to IP
                unsigned int gwAddr;
                std::stringstream ss;
                ss << std::hex << gateway;
                ss >> gwAddr;

                struct in_addr addr{};
                addr.s_addr = gwAddr;
                char buffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN);

                // Reverse byte order
                std::string gw(buffer);
                auto parts = Utils::ipToUint(gw);
                info.gatewayIp = Utils::uintToIp(parts);
                break;
            }
        }
        routeFile.close();
    }

    // Get public IP
    info.publicIp = getPublicIP();

    return info;
}

std::string ipToCIDR(const std::string& ip, const std::string& mask) {
    const uint32_t ipInt = Utils::ipToUint(ip);
    const uint32_t maskInt = Utils::ipToUint(mask);

    // Calculate network address
    const uint32_t network = ipInt & maskInt;

    // Calculate CIDR prefix length
    int prefixLength = 0;
    uint32_t m = maskInt;
    while (m & 0x80000000) {
        prefixLength++;
        m <<= 1;
    }

    // Return network address with CIDR notation
    return Utils::uintToIp(network) + "/" + std::to_string(prefixLength);
}

std::string getSubnet24(const std::string& ip) {
    if (size_t lastDot = ip.find_last_of('.'); lastDot != std::string::npos) {
        return ip.substr(0, lastDot) + ".0/24";
    }
    return ip + "/24";  // Fallback
}