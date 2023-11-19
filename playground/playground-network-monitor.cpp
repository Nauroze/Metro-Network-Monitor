#include "env.h"
#include "network-monitor.h"
#include "websocket-client.h"
#include "websocket-server.h"

#include <chrono>
#include <string>
#include <fstream>

using NetworkMonitor::BoostWebSocketClient;
using NetworkMonitor::BoostWebSocketServer;
using NetworkMonitor::GetEnvVar;
using NetworkMonitor::NetworkMonitorError;
using NetworkMonitor::NetworkMonitorConfig;

int main()
{
    // Monitor configuration
    NetworkMonitorConfig config {
        GetEnvVar("MNM_SERVER_URL", "metronetwork.tech"),
        GetEnvVar("MNM_SERVER_PORT", "443"),
        MNM_USERNAME,
        MNM_PASSWORD,
        TESTS_CACERT_PEM,
        GetEnvVar("MNM_NETWORK_LAYOUT_FILE_PATH", ""),
        "127.0.0.1", // We use the IP as the server hostname because the client
                     // will connect to 127.0.0.1 directly, without host name
                     // resolution.
        "127.0.0.1",
        8042,
        1.0,
        0.1,
        20,
    };

    // Optional run timeout
    // Default: Oms = run indefinitely
    auto timeoutMs {std::stoi(GetEnvVar("MNM_TIMEOUT_MS", "0"))};

    // Launch the monitor.
    NetworkMonitor::NetworkMonitor<
        BoostWebSocketClient,
        BoostWebSocketServer
    > monitor {};
    auto error {monitor.Configure(config)};
    if (error != NetworkMonitorError::kOk) {
        return -1;
    }

    // Record passenger counts.
    nlohmann::json parsed;
    std::ifstream counts_file(PASSENGER_COUNTS);
    if(counts_file.is_open())
    {
    counts_file >> parsed;    
    auto counts {parsed.get<std::unordered_map<NetworkMonitor::Id, int>>()};
    monitor.SetNetworkCrowding(counts);
    }
    else
    {
        spdlog::error("Passenger counts file not found");
    }

    if (timeoutMs == 0) {
        monitor.Run();
    } else {
        monitor.Run(std::chrono::milliseconds(timeoutMs));
    }
    
    // The disconnection of the StompServer client is an acceptable error code.
    // All other truthy error codes are considered failures.
    auto ec {monitor.GetLastErrorCode()};
    if (ec != NetworkMonitorError::kOk &&
        ec != NetworkMonitorError::kStompServerClientDisconnected
    ) {
        spdlog::error("Last error code: {}", ec);
        return -2;
    }
    return 0;
}