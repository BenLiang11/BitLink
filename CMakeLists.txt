cmake_minimum_required(VERSION 3.10.0)
project(i-am-steve)

# Prevent in-source builds to avoid clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed.
    Please create a separate build directory and run CMake from there.
    You may need to remove CMakeCache.txt.")
endif()

# Enable debug mode by default when building inside a dev shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in development shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output compiled binaries to a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable CMake testing
include(CTest)
enable_testing()

# Boost settings
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# Server executable
add_executable(server src/main.cc src/session.cc src/server.cc src/config_parser.cc)

# config_parser related
add_library(config_parser src/config_parser.cc)
add_executable(config_parser_main src/config_parser_main.cc)
target_link_libraries(config_parser_main config_parser)

# server/session related
add_library(server_lib src/server.cc src/session.cc src/config_parser.cc)
add_library(session_lib src/session.cc)

# GoogleTest and test setup (excluded in Docker build)
include(GoogleTest)
# add_subdirectory(/usr/src/googletest googletest)
if(NOT DOCKER_BUILD)
  include(GoogleTest)
  add_subdirectory(third_party/googletest)

  add_executable(config_parser_test tests/config_parser_test.cc)
  target_link_libraries(config_parser_test config_parser gtest_main)
  gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  include(cmake/CodeCoverageReportConfig.cmake)
  generate_coverage_report(TARGETS server config_parser TESTS config_parser_test)
endif()

# server_test
add_executable(server_test tests/server_test.cc)
target_link_libraries(server_test server_lib gtest_main gmock_main)
gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# session_test
add_executable(session_test tests/session_test.cc)
target_link_libraries(session_test session_lib gtest_main gmock_main)
gtest_discover_tests(session_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Code coverage settings (also excluded in Docker)
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(TARGETS session_lib server_lib server config_parser TESTS config_parser_test server_test session_test)
