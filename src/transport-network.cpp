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
    
    /*! \brief Populate the network from a JSON object.
     *
     *  \param src Ownership of the source JSON object is moved to this method.
     *
     *  \returns false if stations and lines where parsed successfully, but not
     *           the travel times.
     *
     *  \throws std::runtime_error This method throws if the JSON items were
     *                             parsed correctly but there was an issue
     *                             adding new stations or lines to the network.
     *  \throws nlohmann::json::exception If there was a problem parsing the
     *                                    JSON object.
     */
    bool TransportNetwork::FromJson(
        nlohmann::json&& src
    )
    {
        bool ok{true};

        // Add Stations
        for(auto&& stationJson : src.at("stations")) {
            Station station {
                std::move(stationJson.at("station_id").get<std::string>()),
                std::move(stationJson.at("name").get<std::string>())
            };
            ok &= AddStation(station);
            if (!ok) {
                throw std::runtime_error("Could not add station " + station.id );
            }
        }

        // Add Lines
        for (auto&& lineJson : src.at("lines")) {
            Line line {
                std::move(lineJson.at("line_id").get<std::string>()),
                std::move(lineJson.at("name").get<std::string>()),
                {} // routes will be added
            };
            // Add Routes to Line
            line.routes.reserve(lineJson.at("routes").size());
            for(auto&& routeJson: lineJson.at("routes")) {
                line.routes.emplace_back(Route {
                    std::move(routeJson.at("route_id").get<std::string>()),
                    std::move(routeJson.at("direction").get<std::string>()),
                    std::move(routeJson.at("line_id").get<std::string>()),
                    std::move(routeJson.at("start_station_id").get<std::string>()),
                    std::move(routeJson.at("end_station_id").get<std::string>()),
                    std::move(
                        routeJson.at("route_stops").get<std::vector<std::string>>()
                    ),
                 });
            }
            ok &= AddLine(line);
            if (!ok) {
                throw std::runtime_error("Could not add line " + line.id);
            }
        }

        // Set Travel Times
        for (auto&& travelTimeJson: src.at("travel_times")) {
            ok &= SetTravelTime(
                std::move(travelTimeJson.at("start_station_id").get<std::string>()),
                std::move(travelTimeJson.at("end_station_id").get<std::string>()),
                std::move(travelTimeJson.at("travel_time").get<unsigned int>())
            );
        }

        return ok;
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

    /*! \brief Get list of routes serving a given station.
     *
     *  \returns An empty vector if there was an error getting the list of
     *           routes serving the station, or if the station has legitimately
     *           no routes serving it.
     *
     *  The station must already be in the network.
     */
    std::vector<Id> TransportNetwork::GetRoutesServingStation(
        const Id& station
    ) const
    {
        const auto stationNode { GetStation(station) };
        if(stationNode == nullptr)
            return {};

        std::vector<Id> routes {};

        const auto& edges {stationNode->edges};
        for (const auto& edge : edges)
        {
            routes.push_back(edge->route->id);
        }

        // This is bad. The previous loop misses a corner case: The end station of a route does
        // not have any edge containing that route, because we only track the routes
        // that *leave from*, not *arrive to* a certain station.
        // We need to loop over all line routes to check if our station is the end
        // stop of any route.
        // FIXME: In the worst case, we are iterating over all routes for all
        //        lines in the network. We may want to optimize this.
        for (const auto& [_, line]: lines_) {
            for (const auto& [_, route]: line->routes) {
                const auto& endStop {route->stops[route->stops.size() - 1]};
                if (stationNode == endStop) {
                    routes.push_back(route->id);
                }
            }
        }

        return routes;
    }
        
    /*! \brief Set the travel time between 2 adjacent stations.
     *
     *  \returns false if there was an error while setting the travel time
     *           between the two stations.
     *
     *  The travel time is the same for all routes connecting the two stations
     *  directly.
     *
     *  The two stations must be adjacent in at least one line route. The two
     *  stations must already be in the network.
     */
    bool TransportNetwork::SetTravelTime(
        const Id& stationA,
        const Id& stationB,
        const unsigned int travelTime
    )
    {
        // Find the stations
        const auto stationANode {GetStation(stationA)};
        const auto stationBNode {GetStation(stationB)};
        if (stationANode == nullptr || stationBNode == nullptr)
        {
            return false;
        }

        // Search all edges connecting A->B and B->A
        // Making a psuedo function with lambda to avoid code duplication
        bool foundAnyEdge {false};
        auto setTravelTime {[&foundAnyEdge, &travelTime](auto from, auto to) {
            for (auto& edge: from->edges) {
                if (edge->nextStop == to) {
                    edge->travelTime = travelTime;
                    foundAnyEdge = true;
                }
            }
        }};
        setTravelTime(stationANode, stationBNode);
        setTravelTime(stationBNode, stationANode);

        return foundAnyEdge;
    }

    /*! \brief Get the travel time between 2 adjacent stations.
     *
     *  \returns 0 if the function could not find the travel time between the
     *           two stations, or if station A and B are the same station.
     *
     *  The travel time is the same for all routes connecting the two stations
     *  directly.
     *
     *  The two stations must be adjacent in at least one line route. The two
     *  stations must already be in the network.
     */
    unsigned int TransportNetwork::GetTravelTime(
        const Id& stationA,
        const Id& stationB
    ) const
    {
        // Find the stations.
        const auto stationANode {GetStation(stationA)};
        const auto stationBNode {GetStation(stationB)};
        if (stationANode == nullptr || stationBNode == nullptr) {
            return 0;
        }

        // Check if there is an edge A -> B, then B -> A.
        // We can return early as soon as we find a match: Wwe know that the travel
        // time from A to B is the same as the travel time from B to A, across all
        // routes.
        for (const auto& edge: stationANode->edges) {
            if (edge->nextStop == stationBNode) {
                return edge->travelTime;
            }
        }
        for (const auto& edge: stationBNode->edges) {
            if (edge->nextStop == stationANode) {
                return edge->travelTime;
            }
        }
        return 0;
    }

    /*! \brief Get the total travel time between any 2 stations, on a specific
     *         route.
     *
     *  The total travel time is the cumulative sum of the travel times between
     *  all stations between `stationA` and `stationB`.
     *
     *  \returns 0 if the function could not find the travel time between the
     *           two stations, or if station A and B are the same station.
     *
     *  The two stations must be both served by the `route`. The two stations
     *  must already be in the network.
     */
    unsigned int TransportNetwork::GetTravelTime(
        const Id& line,
        const Id& route,
        const Id& stationA,
        const Id& stationB
    ) const
    {
        // Find the route
        const auto routeInternal {GetRoute(line, route)};
        if (routeInternal == nullptr)
            return 0;

        // Find the stations
        const auto stationANode {GetStation(stationA)};
        const auto stationBNode {GetStation(stationB)};
        if (stationANode == nullptr || stationBNode == nullptr)
            return 0;

        // Go through the route looking for station A
        unsigned int travelTime {0};
        bool foundA {false};
        for (const auto& stop: routeInternal->stops) {
            // If we found station A, we start counting.
            if (stop == stationANode) {
                foundA = true;
            }

            // If we found station B, we return the cumulative travel time so far.
            if (stop == stationBNode) {
                return travelTime;
            }

            // Accummulate the travel time since we found station A..
            if (foundA) {
                auto edgeIt {stop->FindEdgeForRoute(routeInternal)};
                if (edgeIt == stop->edges.end()) {
                    // Unexpected: The station should definitely have an edge for
                    //             this route.
                    return 0;
                }
                travelTime += (*edgeIt)->travelTime;
            }
        }

        // Did not find either station
        return 0;
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

    std::vector<
        std::shared_ptr<TransportNetwork::GraphEdge>
    >::const_iterator TransportNetwork::GraphNode::FindEdgeForRoute(
        const std::shared_ptr<RouteInternal>& route
    ) const
    {
        return std::find_if(
            edges.begin(),
            edges.end(),
            [&route](const auto& edge) {
                return edge->route == route;
            }
        );
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

    std::shared_ptr<TransportNetwork::LineInternal> TransportNetwork::GetLine(
        const Id& lineId
    ) const
    {
        auto lineIt {lines_.find(lineId)};
        if (lineIt == lines_.end()) {
            return nullptr;
        }
        return lineIt->second;
    }

    std::shared_ptr<TransportNetwork::RouteInternal> TransportNetwork::GetRoute(
        const Id& lineId,
        const Id& routeId
    ) const
    {
        auto line {GetLine(lineId)};
        if (line == nullptr) {
            return nullptr;
        }
        const auto& routes {line->routes};
        auto routeIt {routes.find(routeId)};
        if (routeIt == routes.end()) {
            return nullptr;
        }
        return routeIt->second;
    }
} // NetworkMonitor