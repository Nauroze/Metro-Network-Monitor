cmake_minimum_required(VERSION 3.0.0)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# NETWORK-MONITOR STATIC LIBRARY
project(network-monitor)

find_package(Boost 1.78 REQUIRED COMPONENTS system)

find_package(OpenSSL REQUIRED)

set(SOURCES 
   "${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-client.cpp"
   )
add_library(network-monitor STATIC ${SOURCES})

target_compile_features(network-monitor PUBLIC cxx_std_17)

target_include_directories(network-monitor PUBLIC include/)

target_link_libraries(network-monitor PUBLIC 
                      OpenSSL::OpenSSL
                      Boost::Boost)

if(MINGW)
  target_link_libraries(network-monitor PUBLIC ws2_32)
endif()

# TEST PROGRAM FOR NETWORK-MONITOR
project(network-monitor-test)

set(TEST_SOURCES 
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp"
   )

add_executable(network-monitor-test ${TEST_SOURCES})

target_compile_features(network-monitor-test PRIVATE cxx_std_17)

target_compile_definitions(network-monitor-test PRIVATE 
                          TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem")
                          
target_include_directories(network-monitor-test PRIVATE include/)

target_link_libraries(network-monitor-test PRIVATE
                      network-monitor
                      Boost::Boost)

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