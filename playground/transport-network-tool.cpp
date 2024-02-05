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
    
    // Add 10 people to Victoria Station
    nw.SetNetworkCrowding({{"station_079", 10}});
    
    // St James Park (station 80) to Oxford Cirus (station 18)
    auto travelRoute = nw.GetQuietTravelRoute(
        "station_080",
        "station_018",
        0.2, // max slowdown percentage
        0.2, // min quietness percentage
        10);

    // Open a file in write mode
    std::ofstream file(EXAMPLE_NETWORK_RESULTS);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing" << std::endl;
        return -1;
    }

    // Write the JSON to the file
    nlohmann::json to_file;
    NetworkMonitor::to_json(to_file, travelRoute);
    file << to_file.dump(4);
    file.close();

    std::cout << "JSON data written to example_network_layout.json" << std::endl;

    return 0;
}