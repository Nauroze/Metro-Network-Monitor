#include "transport-network.h"
#include "file-downloader.h"

#include <iostream>
#include <filesystem>
#include <fstream>

using namespace NetworkMonitor;

int main()
{
    TransportNetwork nw;
    auto j = ParseJsonFile(std::filesystem::path(EXAMPLE_NETWORK_LAYOUT));
    bool ok = nw.FromJson(std::move(j));
    if (!ok)
    {
        std::cerr << "JSON file invalid\n";
        return -1;
    }
    nw.SetNetworkCrowding({{"station_0",1}});
    auto travelRoute = nw.GetQuietTravelRoute(
        "station_0",
        "station_1",
        1.0, // max slow down percent
        0.1  // min quiet
    );
    
    // Open a file in write mode
    std::ofstream file(EXAMPLE_NETWORK_RESULTS);
    // Check if the file is open
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing" << std::endl;
        return -1;
    }

    // Write the JSON to the file
    nlohmann::json to_file;
    NetworkMonitor::to_json(to_file, travelRoute);
    file << to_file.dump(4);

    // Close the file
    file.close();

    std::cout << "JSON data written to example_network_layout.json" << std::endl;

    return 0;
}