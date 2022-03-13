cmake_minimum_required(VERSION 3.0.0)
project(network-monitor)

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()

find_package(Boost 1.78 REQUIRED COMPONENTS system)

set(SOURCES 
   "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
   "${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-client.cpp"
   )

add_executable(network-monitor ${SOURCES})

target_compile_features(network-monitor PRIVATE cxx_std_17)

target_include_directories(network-monitor PRIVATE include/)

target_link_libraries(network-monitor PRIVATE Boost::Boost)

if(MINGW)
  target_link_libraries(network-monitor PRIVATE ws2_32)
endif()