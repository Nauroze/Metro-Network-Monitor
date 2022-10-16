#include "transport-network.h"

namespace NetworkMonitor {

    TransportNetwork::TransportNetwork() = default;

    TransportNetwork::~TransportNetwork() = default;

    TransportNetwork::TransportNetwork(
        const TransportNetwork& copied
    )
    {

    }

    TransportNetwork::TransportNetwork(
        TransportNetwork&& moved
    )
    {
        
    }

    /*! \brief Add a station to the network.
     *
     *  \returns false if there was an error while adding the station to the
     *           network.
     *
     *  This function assumes that the Station object is well-formed.
     *
     *  The station cannot already be in the network.
     */
    bool TransportNetwork::AddStation(
        const Station& station
    )
    {
        // Check if map already has this station
        if (stations_.find(station.id) != stations_.end())
            return false;

        // Crate std::shared_ptr<GraphNode>
        auto station_node {std::make_shared<GraphNode>(GraphNode {
            station.id,
            station.name,
            0, 
            {} 
        })};
        
        // Add shared_ptr of GraphNode to stations_ map.
        stations_.emplace(station.id, std::move(station_node));

        return true;
    }
} // NetworkMonitor