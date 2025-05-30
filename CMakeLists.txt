cmake_minimum_required(VERSION 3.31)
project(rms LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include/rms)

# Source files
file(GLOB_RECURSE RMS_SRC
    src/*.cpp
)

add_executable(rms_core main.cpp ${RMS_SRC}
        tests/pretrade_checks_test.cpp
)

find_package(yaml-cpp REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
# Assume Aeron and Prometheus client libs are installed or stubbed
# pkg_check_modules(AERON aeron-cpp)
# include_directories(${AERON_INCLUDE_DIRS})
# link_libraries(${AERON_LIBRARIES})

target_link_libraries(rms_core yaml-cpp Threads::Threads)

# Unit tests
add_subdirectory(tests)
