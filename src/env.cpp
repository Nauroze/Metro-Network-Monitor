#include "env.h"

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

std::string NetworkMonitor::GetEnvVar(
    const std::string& envVar,
    const std::optional<std::string>& defaultValue
)
{
    const char* value {std::getenv(envVar.c_str())};
    if (value != nullptr) {
        return value;
    }
    if (defaultValue.has_value()) {
        return *defaultValue;
    }
    throw std::runtime_error("Could not find environment variable: " + envVar);
}