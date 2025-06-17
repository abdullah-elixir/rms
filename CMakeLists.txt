cmake_minimum_required(VERSION 3.16)
project(rms LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include/rms)


# Source files
file(GLOB_RECURSE RMS_SRC
    src/*.cpp
)

# Remove a specific file from the list
list(REMOVE_ITEM RMS_SRC
    src/persistence.cpp
)

add_executable(rms_core main.cpp ${RMS_SRC}
        include/rms/aeronWrapper.h
        include/rms/sharded_queue.h
        src/sharded_queue.cpp
        include/rms/utils/params.h
        include/rms/logger.h
)
# Required so folly can locate its dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/share/cmake-4.0/Modules")
find_package(yaml-cpp REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(aeron REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
find_package(folly REQUIRED)
find_package(Boost REQUIRED)

target_link_libraries(rms_core 
    yaml-cpp 
    Threads::Threads 
    folly 
    aeron 
    aeron_client 
    GTest::GTest 
    GTest::Main 
    glog 
    fmt
    /usr/local/lib/librocksdb.a
    zstd
    snappy
    bz2
    lz4
    double-conversion
    z
    spdlog
)

# Unit tests
add_subdirectory(tests)