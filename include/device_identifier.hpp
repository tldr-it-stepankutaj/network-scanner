#pragma once

#include <string>
#include <map>

class DeviceIdentifier {
public:
    DeviceIdentifier();
    std::string identifyDevice(const std::string& ip);

private:
    std::map<std::string, std::string> macToVendor;
    std::map<int, std::string> portToService;

    static std::string resolveHostname(const std::string& ip);
    std::string checkCommonServices(const std::string& ip);
    static std::string identifyByPattern(const std::string& ip);
    bool verifyHost(const std::string& ip);
    std::string lookupMacVendor(const std::string& ip);
};
