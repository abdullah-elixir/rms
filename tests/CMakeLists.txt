cmake_minimum_required(VERSION 3.16)
project(rms_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)
find_package(folly REQUIRED)
find_package(fmt REQUIRED)
find_package(glog REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ../include)


add_executable(pretrade_checks_test pretrade_checks_test.cpp ../src/pretrade_checks.cpp ../src/data_types.cpp)
add_executable(vcm_module_test vcm_module_test.cpp ../src/vcm_module.cpp ../src/data_types.cpp)
add_executable(posttrade_controls_test posttrade_controls_test.cpp ../src/posttrade_controls.cpp ../src/data_types.cpp)

target_link_libraries(pretrade_checks_test GTest::GTest GTest::Main pthread yaml-cpp folly fmt::fmt glog::glog)
target_link_libraries(vcm_module_test GTest::GTest GTest::Main pthread yaml-cpp folly fmt::fmt glog::glog)
target_link_libraries(posttrade_controls_test GTest::GTest GTest::Main pthread yaml-cpp folly fmt::fmt glog::glog)

# Integration test stub
add_executable(integration_test integration_test.cpp ../src/pretrade_checks.cpp ../src/vcm_module.cpp ../src/posttrade_controls.cpp ../src/data_types.cpp)
target_link_libraries(integration_test GTest::GTest GTest::Main pthread yaml-cpp folly fmt::fmt glog::glog)