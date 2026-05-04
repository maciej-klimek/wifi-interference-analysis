cmake_minimum_required(VERSION 3.10)
project(WiFiBluetooth-Interference-Study)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to ns-3 build directory
set(NS3_BUILD_DIR "${CMAKE_SOURCE_DIR}/../ns3/build" CACHE PATH "Path to ns-3 build directory")
set(NS3_LIB_DIR "${NS3_BUILD_DIR}/lib")
set(NS3_INCLUDE_DIR "${NS3_BUILD_DIR}/include")

# Check if ns3 is built
if(NOT EXISTS "${NS3_LIB_DIR}")
    message(FATAL_ERROR "ns3 build directory not found at ${NS3_BUILD_DIR}\n"
                        "Please build ns3 first:\n"
                        "  cd ../ns3\n"
                        "  ./ns3 configure --enable-examples\n"
                        "  ./ns3 build")
endif()

# Add the simulation executable
add_executable(bt-wifi-interference-sim src/bt-wifi-interference-sim.cc)
add_executable(test-simple src/test-simple.cc)

# Add include directories
target_include_directories(bt-wifi-interference-sim PRIVATE "${NS3_INCLUDE_DIR}")
target_include_directories(test-simple PRIVATE "${NS3_INCLUDE_DIR}")

# Link against ns3 libraries - link all needed modules
target_link_directories(bt-wifi-interference-sim PRIVATE "${NS3_LIB_DIR}")
target_link_directories(test-simple PRIVATE "${NS3_LIB_DIR}")

# Link all necessary ns3 libraries - main sim
target_link_libraries(bt-wifi-interference-sim PRIVATE
    ns3-dev-core-default
    ns3-dev-network-default
    ns3-dev-internet-default
    ns3-dev-wifi-default
    ns3-dev-mobility-default
    ns3-dev-applications-default
    ns3-dev-propagation-default
    ns3-dev-spectrum-default
    ns3-dev-stats-default
    pthread
    m
)

# Link libraries for test
target_link_libraries(test-simple PRIVATE
    ns3-dev-core-default
    ns3-dev-network-default
    ns3-dev-internet-default
    ns3-dev-wifi-default
    ns3-dev-mobility-default
    ns3-dev-propagation-default
    pthread
    m
)

# Set output directory
set_target_properties(bt-wifi-interference-sim PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_target_properties(test-simple PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
