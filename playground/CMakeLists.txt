# Playground applications
add_executable(playground-client "playground-client.cpp")

target_compile_definitions(playground-client
    PRIVATE
        LTNM_USERNAME="naurozehoath@gmail.com"
        LTNM_PASSWORD="9ba4efcec11a271f9021a461a8018c9fa8bfc33c"
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/test-data"
        START_END_JSON="${CMAKE_CURRENT_SOURCE_DIR}/playground/start_end_stations.json"
        QUIET_ROUTE_JSON="${CMAKE_CURRENT_SOURCE_DIR}/playground/quiet_route_result.json" 
)

target_link_libraries(playground-client
    PRIVATE
        network-monitor
        nlohmann_json::nlohmann_json
        OpenSSL::OpenSSL
        spdlog::spdlog
)

add_executable(playground-network-monitor "playground-network-monitor.cpp")

target_compile_definitions(playground-network-monitor
    PRIVATE
        TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/test-data"
        NETWORK_LAYOUT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json"
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        PASSENGER_COUNTS_JSON="${CMAKE_CURRENT_SOURCE_DIR}/playground/passenger_counts.json"
        DISABLE_NETWORK_EVENTS 
)

target_link_libraries(playground-network-monitor
    PRIVATE
        network-monitor
        nlohmann_json::nlohmann_json
        OpenSSL::OpenSSL
        spdlog::spdlog
)