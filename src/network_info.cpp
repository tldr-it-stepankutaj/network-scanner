#include "../include/network_info.hpp"
#include "../include/utils.hpp"
#include "../include/logger.hpp"

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

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/wait.h>
#endif

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
            Logger::debug("Failed to get public IP: " + std::string(curl_easy_strerror(res)));
            return "Error: " + std::string(curl_easy_strerror(res));
        }
    }

    Logger::debug("Public IP: " + readBuffer);
    return readBuffer;
}

NetworkInfo getNetworkInfo() {
    NetworkInfo info;

    // Get local IP and interface name
    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) == 0) {
        for (struct ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                if (strcmp(ifa->ifa_name, "lo") == 0 || strcmp(ifa->ifa_name, "lo0") == 0) continue;

                auto* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                char buffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa->sin_addr), buffer, INET_ADDRSTRLEN);

                std::string ip(buffer);
                if (ip.find("127.") == 0 || ip.find("169.254.") == 0) continue;

                info.localIp = ip;
                info.interfaceName = ifa->ifa_name;

                if (ifa->ifa_netmask) {
                    auto* mask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);
                    char maskBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(mask->sin_addr), maskBuffer, INET_ADDRSTRLEN);
                    info.subnetMask = maskBuffer;
                }

                Logger::debug("Detected interface: " + info.interfaceName + " IP: " + info.localIp + " mask: " + info.subnetMask);
                break;
            }
        }
        freeifaddrs(ifap);
    }

    // Get gateway IP
#ifdef __linux__
    if (std::ifstream routeFile("/proc/net/route"); routeFile.is_open()) {
        std::string line;
        std::getline(routeFile, line); // skip header

        while (std::getline(routeFile, line)) {
            std::istringstream iss(line);
            std::string iface, dest, gateway;
            iss >> iface >> dest >> gateway;

            if (dest == "00000000" && iface == info.interfaceName) {
                unsigned int gwAddr;
                std::stringstream ss;
                ss << std::hex << gateway;
                ss >> gwAddr;

                struct in_addr addr{};
                addr.s_addr = gwAddr;
                char buffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr, buffer, INET_ADDRSTRLEN);

                std::string gw(buffer);
                auto parts = Utils::ipToUint(gw);
                info.gatewayIp = Utils::uintToIp(parts);
                Logger::debug("Gateway detected (Linux): " + info.gatewayIp);
                break;
            }
        }
        routeFile.close();
    }
#elif defined(__APPLE__)
    // macOS: use route command via fork/exec
    {
        int pipefd[2];
        if (pipe(pipefd) == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                int devnull = open("/dev/null", O_RDWR);
                if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }
                execlp("route", "route", "-n", "get", "default", nullptr);
                _exit(127);
            } else if (pid > 0) {
                close(pipefd[1]);
                char buf[4096];
                std::string output;
                ssize_t n;
                while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
                    buf[n] = '\0';
                    output += buf;
                }
                close(pipefd[0]);
                int status;
                waitpid(pid, &status, 0);

                // Parse "gateway: x.x.x.x" from output
                std::istringstream oss(output);
                std::string line;
                while (std::getline(oss, line)) {
                    auto pos = line.find("gateway:");
                    if (pos != std::string::npos) {
                        std::string gw = line.substr(pos + 8);
                        // trim whitespace
                        gw.erase(0, gw.find_first_not_of(" \t"));
                        gw.erase(gw.find_last_not_of(" \t\r\n") + 1);
                        if (Utils::isValidIpv4(gw)) {
                            info.gatewayIp = gw;
                            Logger::debug("Gateway detected (macOS): " + info.gatewayIp);
                        }
                        break;
                    }
                }
            } else {
                close(pipefd[0]);
                close(pipefd[1]);
            }
        }
    }
#endif

    if (info.gatewayIp.empty()) {
        Logger::debug("Gateway not detected");
    }

    info.publicIp = getPublicIP();

    return info;
}

std::string ipToCIDR(const std::string& ip, const std::string& mask) {
    const uint32_t ipInt = Utils::ipToUint(ip);
    const uint32_t maskInt = Utils::ipToUint(mask);

    const uint32_t network = ipInt & maskInt;

    int prefixLength = 0;
    uint32_t m = maskInt;
    while (m & 0x80000000) {
        prefixLength++;
        m <<= 1;
    }

    return Utils::uintToIp(network) + "/" + std::to_string(prefixLength);
}

std::string getSubnet24(const std::string& ip) {
    if (size_t lastDot = ip.find_last_of('.'); lastDot != std::string::npos) {
        return ip.substr(0, lastDot) + ".0/24";
    }
    return ip + "/24";
}
