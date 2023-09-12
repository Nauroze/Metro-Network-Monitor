#include "env.h"
#include "network-monitor.h"
#include "websocket-client.h"
#include "websocket-server.h"

#include <chrono>
#include <string>

using NetworkMonitor::BoostWebSocketClient;
using NetworkMonitor::BoostWebSocketServer;
using NetworkMonitor::GetEnvVar;
using NetworkMonitor::NetworkMonitorError;
using NetworkMonitor::NetworkMonitorConfig;


/*  
                      (100)
    Route 0:    1---2---3---4---5
                       
    Route 1:  20---1---21---22---4---23
                      (50)
    
    Route 0 has 1 travel times between each station
    Route 1 has 2 travel times between each station
*/
int main()
{
    // Monitor configuration
    NetworkMonitorConfig config {
        GetEnvVar("LTNM_SERVER_URL", "ltnm.learncppthroughprojects.com"),
        GetEnvVar("LTNM_SERVER_PORT", "443"),
        LTNM_USERNAME,
        LTNM_PASSWORD,
        TESTS_CACERT_PEM,
        PLAYGROUND_LAYOUT_FILE, //network_fastest_path_2routes.json
        "127.0.0.1", // We use the IP as the server hostname because the client
                     // will connect to 127.0.0.1 directly, without host name
                     // resolution.
        "127.0.0.1",
        8042,
        1.0, // Max Slowdown Percentage. i.e I am okay to take a route that will take twice as long the fastest route provided minimum percentage decrease in crowding is met.
        0.1, // Min Percentage Decrease In Crowd. i.e I'll take this route as long as it's 10% or more quieter than the fastest route.
        20,
    };

    // Optional run timeout
    // Default: Oms = run indefinitely
    auto timeoutMs {std::stoi(GetEnvVar("LTNM_TIMEOUT_MS", "0"))};

    // Launch the monitor.
    NetworkMonitor::NetworkMonitor<
        BoostWebSocketClient,
        BoostWebSocketServer
    > monitor {};
    auto error {monitor.Configure(config)};
    if (error != NetworkMonitorError::kOk) {
        return -1;
    }
    
    // Induce passenger count
    std::unordered_map<NetworkMonitor::Id, int> passengerCounts;
    passengerCounts["station_3"] = 100;
    passengerCounts["station_21"] = 50;
    monitor.SetNetworkCrowding(passengerCounts);
    
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