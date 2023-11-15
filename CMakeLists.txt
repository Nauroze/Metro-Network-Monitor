cmake_minimum_required(VERSION 3.0.0)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# NETWORK-MONITOR STATIC LIBRARY
project(network-monitor)

set (CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0")

find_package(Boost 1.78 REQUIRED COMPONENTS system)

find_package(OpenSSL REQUIRED)

find_package(CURL REQUIRED)

find_package(nlohmann_json REQUIRED)

find_package(spdlog REQUIRED)

set(SOURCES 
   "${CMAKE_CURRENT_SOURCE_DIR}/src/env.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/file-downloader.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/network-monitor.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/stomp-client.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/stomp-frame.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/stomp-server.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/transport-network.cpp"
   )

add_library(network-monitor STATIC ${SOURCES})

target_compile_features(network-monitor PUBLIC cxx_std_17)

target_include_directories(network-monitor PUBLIC include/)

target_link_libraries(network-monitor 
                    PUBLIC 
                      OpenSSL::OpenSSL
                      Boost::Boost
                      nlohmann_json::nlohmann_json
                      spdlog::spdlog
                    PRIVATE
                      CURL::CURL)

if(MINGW)
  target_link_libraries(network-monitor PUBLIC ws2_32)
endif()

# TEST PROGRAM FOR NETWORK-MONITOR
project(network-monitor-test)

set(TEST_SOURCES 
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/file-downloader.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/network-monitor.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-client.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-frame.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-server.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/transport-network.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client-mock.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-server.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-server-mock.cpp"
   )

add_executable(network-monitor-test ${TEST_SOURCES})

target_compile_features(network-monitor-test PRIVATE cxx_std_17)

target_compile_definitions(network-monitor-test 
                          PRIVATE
                            MNM_USERNAME="valid_user"
                            MNM_PASSWORD="valid_password"
                            TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/test-data" 
                            TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
                            TESTS_NETWORK_LAYOUT_JSON="${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json")
                          
target_include_directories(network-monitor-test PRIVATE include/)

target_link_libraries(network-monitor-test PRIVATE
                      network-monitor
                      Boost::Boost
                      nlohmann_json::nlohmann_json
                      spdlog::spdlog
                      )

if(MINGW)
  target_link_libraries(network-monitor-test PRIVATE ws2_32)
endif()

# ENABLE CMAKE TESTS
enable_testing()

add_test(
  NAME network-monitor-test
  COMMAND $<TARGET_FILE:network-monitor-test>
)
set_tests_properties(network-monitor-test PROPERTIES
    PASS_REGULAR_EXPRESSION ".*No errors detected"
)

# Network Monitor Executable
set(EXE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)
add_executable(network-monitor-exe ${EXE_SOURCES})
target_compile_features(network-monitor-exe
    PRIVATE
        cxx_std_17
)
target_compile_definitions(network-monitor-exe
    PRIVATE
        MNM_USERNAME="valid_user"
        MNM_PASSWORD="valid_password"
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=${WINDOWS_VERSION}>
)
target_compile_options(network-monitor-exe
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_link_libraries(network-monitor-exe
    PRIVATE
        network-monitor
        spdlog::spdlog
)
add_test(
    NAME network-monitor-exe-tests
    COMMAND $<TARGET_FILE:network-monitor-exe>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)
# These options define the pass/fail condition for the test.
set_tests_properties(network-monitor-exe-tests PROPERTIES
    # This affects the duration of the io_context::run function.
    ENVIRONMENT "MNM_TIMEOUT_MS=5000"

    # We introduce a timeout just to be safe.
    # This should be longer than MNM_TIMEOUT_MS.
    TIMEOUT 10

    # We verify the content of the program log output. Any error message is a
    # test failure.
    FAIL_REGULAR_EXPRESSION "\\[error\\]"
)

# Test executables
# We build a test STOMP client and then run it in parallel with the network
# monitor executable. We use an intermediate CMake script to run the two
# executables in parallel.
set(TEST_CLIENT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_client.cpp"
)
add_executable(test-client-exe ${TEST_CLIENT_SOURCES})
target_compile_features(test-client-exe
    PRIVATE
        cxx_std_17
)
target_compile_definitions(test-client-exe
    PRIVATE
        MNM_USERNAME="valid_user"
        MNM_PASSWORD="valid_password"
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=${WINDOWS_VERSION}>
)
target_compile_options(test-client-exe
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_link_libraries(test-client-exe
    PRIVATE
        network-monitor
        nlohmann_json::nlohmann_json
        OpenSSL::OpenSSL
        spdlog::spdlog
)
add_test(
    NAME integration-test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    COMMAND "${CMAKE_COMMAND}"
        -DNETWORK_MONITOR_EXE=$<TARGET_FILE:network-monitor-exe>
        -DTEST_CLIENT_EXE=$<TARGET_FILE:test-client-exe>
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration-test.cmake"
)
# These options define the pass/fail condition for the test.
set_tests_properties(integration-test PROPERTIES
    # This affects the duration of the io_context::run function.
    ENVIRONMENT "MNM_TIMEOUT_MS=20000"

    # We introduce a timeout just to be safe.
    # This should be longer than MNM_TIMEOUT_MS.
    TIMEOUT 30

    # We verify the content of the program log output. Any error message is a
    # test failure.
    FAIL_REGULAR_EXPRESSION "\\[error\\]"
)

# Playground applications
add_executable(playground-client "${CMAKE_CURRENT_SOURCE_DIR}/playground/playground-client.cpp")

target_compile_definitions(playground-client
    PRIVATE
        MNM_USERNAME="valid_user"
        MNM_PASSWORD="valid_password"
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/test-data" 
)

target_link_libraries(playground-client
    PRIVATE
        network-monitor
        nlohmann_json::nlohmann_json
        OpenSSL::OpenSSL
        spdlog::spdlog
)

add_executable(playground-network-monitor "${CMAKE_CURRENT_SOURCE_DIR}/playground/playground-network-monitor.cpp")

target_compile_definitions(playground-network-monitor
    PRIVATE
        MNM_USERNAME="valid_user"
        MNM_PASSWORD="valid_password"
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/test-data" 
)

target_link_libraries(playground-network-monitor
    PRIVATE
        network-monitor
        nlohmann_json::nlohmann_json
        OpenSSL::OpenSSL
        spdlog::spdlog
)