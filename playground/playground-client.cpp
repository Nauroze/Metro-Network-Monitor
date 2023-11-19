#include <iostream>
#include <string>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <stomp-client.h>
#include <transport-network.h>
#include <websocket-client.h>

int main()
{
    // Create a STOMP client
    const std::string url {"127.0.0.1"};
    const std::string endpoint {"/quiet-route"};
    const std::string port {"8042"};
    const std::string username {"username"};
    const std::string password {"password"};
    boost::asio::io_context ioc {};
    boost::asio::ssl::context ctx { boost::asio::ssl::context::tlsv12_client };
    ctx.load_verify_file(TESTS_CACERT_PEM);
    NetworkMonitor::StompClient<NetworkMonitor::BoostWebSocketClient> client {
        url,
        endpoint,
        port,
        ioc,
        ctx
    };

    // After onConnect and after message is sent, call onSend.
    // Does not really do anything, but is a requirement for client.Send()
    auto onSend{
        [](auto ec, auto&& id){
            if(ec!=NetworkMonitor::StompClientError::kOk)
            {
                spdlog::error("QuietRouteClient: Failed to send message");
                throw std::runtime_error("Failed");
            }
            spdlog::info("QuietRouteClient: /quiet-route request sent");
        }
    };
    //When connected, send the quiet route request to server.
    auto onConnect{
        [&client, &onSend](auto ec){
            if(ec!=NetworkMonitor::StompClientError::kOk)
            {
                spdlog::error("QuietRouteClient: Could not connect to server");
                throw std::runtime_error("Failed");
            }
            spdlog::info("QuietRouteClient: Connected");
            
            std::string start_number, end_number;
            std::cout << "Enter Starting Station Number:";
            std::cin >> start_number;
            std::string start_station("station_"+start_number);
            std::cout << "Enter End Station Number:";
            std::cin >> end_number;
            std::string end_station("station_"+end_number);
            const std::string message {
                nlohmann::json {
                    {"start_station_id", start_station},
                    {"end_station_id", end_station},
                }.dump()
            };
            
            client.Send("/quiet-route",message,onSend);
        }
    };

    auto onClose{
        [](auto ec){
            spdlog::info("QuietRouteClient: Connection closed");
        }
    };

    auto onMessage{
        [&client, &onClose](auto ec, auto dst, auto&& message){
            if(ec!=NetworkMonitor::StompClientError::kOk)
            {
                spdlog::error("QuietRouteClient: Error in receiving message from server");
                throw std::runtime_error("Failed");
            }
            spdlog::info("QuietRouteClient: Response received from Server");
            NetworkMonitor::TravelRoute quietRoute{};
            quietRoute = nlohmann::json::parse(message);
            spdlog::info("QuietRouteClient: Travel route received, closing connection.");
            nlohmann::json to_file;            
            NetworkMonitor::to_json(to_file, quietRoute);
            std::ofstream file(RESULTS_FILE);
            file << to_file.dump(4);
            client.Close(onClose);
        }
    };
    client.Connect(username,password,onConnect,onMessage,onClose);
    ioc.run();

    return 0;
}