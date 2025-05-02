#pragma once

#include <string>

struct NetworkInfo {
    std::string localIp;
    std::string gatewayIp;
    std::string publicIp;
    std::string interfaceName;
    std::string subnetMask;
};

/**
 * Get network information including local IP, gateway IP, and public IP
 *
 * @return NetworkInfo structure with network details
 */
NetworkInfo getNetworkInfo();

/**
 * Get public IP address using an external service
 *
 * @return String containing the public IP address
 */
std::string getPublicIP();

/**
 * Convert IP address to CIDR notation based on subnet mask
 *
 * @param ip The IP address
 * @param mask The subnet mask
 * @return String with CIDR notation (e.g., "192.168.1.0/24")
 */
std::string ipToCIDR(const std::string& ip, const std::string& mask);

/**
 * Extract /24 subnet from an IP address
 *
 * @param ip The IP address
 * @return String with /24 CIDR notation (e.g., "192.168.1.0/24")
 */
std::string getSubnet24(const std::string& ip);