#pragma once

#include <string>
#include <map>

/**
 * Class for identifying devices on the network
 *
 * Uses various methods to identify devices, including:
 * - Hostname resolution
 * - Common service port scanning
 * - IP address pattern recognition
 */
class DeviceIdentifier {
public:
    /**
     * Constructor
     *
     * Initializes the port-to-service mapping and loads MAC vendor database if available
     */
    DeviceIdentifier();

    /**
     * Identify a device by its IP address
     *
     * @param ip The IP address to identify
     * @return String with the identified device type or "Unknown Device" if unable to identify
     */
    std::string identifyDevice(const std::string& ip);

private:
    // Map of MAC address prefixes to vendor names
    std::map<std::string, std::string> macToVendor;

    // Map of port numbers to service names
    std::map<int, std::string> portToService;

    /**
     * Try to resolve hostname for an IP address
     *
     * @param ip The IP address
     * @return Hostname or empty string if unable to resolve
     */
    static std::string resolveHostname(const std::string& ip);

    /**
     * Check for common services on the device
     *
     * @param ip The IP address
     * @return Service name or empty string if no common services found
     */
    std::string checkCommonServices(const std::string& ip);

    /**
     * Identify device based on IP address patterns
     *
     * @param ip The IP address
     * @return Device type based on pattern or empty string if no pattern matches
     */
    static std::string identifyByPattern(const std::string& ip);

    /**
     * Verify that a host is truly alive with multiple methods
     *
     * @param ip The IP address to verify
     * @return True if the host is verified to be alive, false otherwise
     */
    static bool verifyHost(const std::string& ip);
};