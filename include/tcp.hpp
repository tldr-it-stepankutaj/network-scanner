#pragma once
#include <string>

namespace Tcp {
    bool ping(const std::string& ip, int port, bool quiet = false, int timeoutMs = 1000);
}
