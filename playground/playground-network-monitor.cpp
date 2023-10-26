#include "network-monitor.h"
#include "websocket-client.h"
#include "websocket-server.h"

using NetworkMonitor::BoostWebSocketClient;
using NetworkMonitor::BoostWebSocketServer;
using NetworkMonitor::NetworkMonitorError;
using NetworkMonitor::NetworkMonitorConfig;

int main()
{
    // Network monitor configuration - mostly default
    NetworkMonitorConfig config;
    config.caCertFile = TESTS_CACERT_PEM;
    config.networkLayoutFile = NETWORK_LAYOUT_FILE;

    // Launch the monitor.
    NetworkMonitor::NetworkMonitor<
        BoostWebSocketClient,
        BoostWebSocketServer
    > monitor {};
    auto error {monitor.Configure(config)};
    if (error != NetworkMonitorError::kOk) {
        return -1;
    }

    // Record a set of passenger counts. 
    std::unordered_map<std::string, int> passengerCounts = NetworkMonitor::ParseJsonFile(
            PASSENGER_COUNTS_JSON
        ).get<std::unordered_map<std::string, int>>();
  
    monitor.SetNetworkCrowding(passengerCounts);

    monitor.Run();
    
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