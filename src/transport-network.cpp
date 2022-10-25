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
        if (GetStation(station.id))
            return false;

        // Create std::shared_ptr<GraphNode>
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

    /*! \brief Add a line to the network.
     *
     *  \returns false if there was an error while adding the line to the
     *           network.
     *
     *  This function assumes that the Line object is well-formed.
     *
     *  All stations served by this line must already be in the network. 
     *  
     *  The line cannot already be in the network.
     */
    bool TransportNetwork::AddLine(
        const Line& line
    )
    {
        // Check if map already has this line
        if (lines_.find(line.id) != lines_.end())
            return false;

        // Create std::shared_ptr<LineInternal>
        auto lineInternal {std::make_shared<LineInternal>(LineInternal {
            line.id,
            line.name,
            {} 
        })};

        // Add the routes to the line.
        for (const auto& route: line.routes) {
            if (!AddRouteToLine(route, lineInternal)) {
                return false;
            }
        }

        // Add shared_ptr of LineInternal to lines_ map.
        lines_.emplace(line.id, std::move(lineInternal));

        return true;
    }

    /*! \brief Record a passenger event at a station.
     *
     *  \returns false if the station is not in the network or if the passenger
     *           event is not reconized.
     */
    bool TransportNetwork::RecordPassengerEvent(
        const PassengerEvent& event
    )
    {
        const auto stationNode { GetStation(event.stationId) };
        
        if(stationNode == nullptr)
            return false;
        
        if (event.type == PassengerEvent::Type::In)
            stationNode->passengerCount++;
        else if(event.type == PassengerEvent::Type::Out)
            stationNode->passengerCount--;
        else
            return false;

        return true;
    }

    long long int TransportNetwork::GetPassengerCount(
        const Id& station
    ) const
    {
        const auto stationNode { GetStation(station) };

        if(stationNode == nullptr)
            std::__throw_runtime_error("Station ID does not match any station \n");

        return stationNode->passengerCount;
    }

    /*! \brief Add a route to lineInternal
     *
     *  \returns false if there was an error while adding the route to the
     *           line.
     */
    bool TransportNetwork::AddRouteToLine(
    const Route& route,
    const std::shared_ptr<LineInternal>& lineInternal
    )
    {
        // Check if route is already in line
        if (lineInternal->routes.find(route.id) != lineInternal->routes.end())
            return false;

        // Check if stations in the route already in the network
        std::vector<std::shared_ptr<GraphNode>> stops {};
        stops.reserve(route.stops.size());
        for (const auto& stopId: route.stops) {
            const auto station {GetStation(stopId)};
            if (station == nullptr) {
                return false;
            }
            stops.push_back(station);
        }

        // Create internal represenation of route
        auto routeInternal {std::make_shared<RouteInternal>(RouteInternal {
            route.id,
            lineInternal,
            std::move(stops)
        })};
        
        // Iterate through the stations associated with this route
        // Add an edge of this route to the station
        for (int i= 0; i < routeInternal->stops.size() - 1; ++i) {
            const auto& thisStop {routeInternal->stops[i]};
            const auto& nextStop {routeInternal->stops[i + 1]};
            thisStop->edges.emplace_back(std::make_shared<GraphEdge>(GraphEdge {
                routeInternal,
                nextStop,
                0,
            }));
        }

        // Finally, add the route to the line.
        lineInternal->routes[route.id] = std::move(routeInternal);

        return true;
    }

    std::shared_ptr<TransportNetwork::GraphNode> TransportNetwork::GetStation(
        const Id& stationId
    ) const
    {
        auto it {stations_.find(stationId)};
        if (it == stations_.end()) {
            return nullptr;
        }
        return it->second;
    }
} // NetworkMonitor