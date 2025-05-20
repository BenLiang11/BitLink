cmake_minimum_required(VERSION 3.10.0)
project(i-am-steve)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Set the C++ standard to C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# boost
include_directories(/opt/homebrew/include) 
link_directories(/opt/homebrew/lib)

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
find_package(Boost 1.50 REQUIRED COMPONENTS system log log_setup date_time)
message(STATUS "Boost version: ${Boost_VERSION}")

# Project source files

# Server executable
add_executable(server src/main.cc)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(server server_lib handler_registry_lib logger_lib)

# Basic libraries (no interdependencies)
add_library(logger_lib src/logger.cc)
target_include_directories(logger_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(logger_lib Boost::log Boost::log_setup Boost::date_time pthread)

add_library(config_parser src/config_parser.cc)
target_include_directories(config_parser PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(request_lib src/request.cc)
target_include_directories(request_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(response_lib src/response.cc)
target_include_directories(response_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(mime_types_lib src/mime_types.cc)
target_include_directories(mime_types_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Registry library
add_library(handler_registry_lib src/handler_registry.cc)
target_include_directories(handler_registry_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Handlers library
add_library(handlers_lib 
            src/handlers/echo_handler.cc 
            src/handlers/static_file_handler.cc
            src/handlers/not_found_handler.cc
            src/handlers/api_handler.cc)
target_include_directories(handlers_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(handlers_lib mime_types_lib request_lib response_lib handler_registry_lib)

# Dispatcher library
add_library(handler_dispatcher_lib src/handler_dispatcher.cc)
target_include_directories(handler_dispatcher_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(handler_dispatcher_lib handler_registry_lib)

# Server config library
add_library(server_config_lib src/server_config.cc)
target_include_directories(server_config_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(server_config_lib handlers_lib config_parser)

# Handler server library
add_library(handler_server_lib src/handler_server.cc)
target_include_directories(handler_server_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(handler_server_lib handler_dispatcher_lib logger_lib)

# Session library
add_library(session_lib src/session.cc)
target_include_directories(session_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(session_lib request_lib response_lib handler_dispatcher_lib Boost::log Boost::log_setup Boost::date_time pthread)

# Server library
add_library(server_lib src/server.cc)
target_include_directories(server_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(server_lib session_lib handler_server_lib server_config_lib)

# Config parser executable
add_executable(config_parser_main src/config_parser_main.cc)
target_include_directories(config_parser_main PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(config_parser_main config_parser)

# GoogleTest and test setup (excluded in Docker build)
include(GoogleTest)
if(NOT DOCKER_BUILD)
  include(GoogleTest)
  add_subdirectory(/usr/src/googletest googletest)


  add_executable(config_parser_test tests/config_parser_test.cc)
  target_link_libraries(config_parser_test config_parser gtest_main)
  gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

# server_test
add_executable(server_test tests/server_test.cc)
target_link_libraries(server_test server_lib gtest_main gmock_main)

# session_test
add_executable(session_test tests/session_test.cc)
target_link_libraries(session_test session_lib gtest_main gmock_main)

# handler_tests
add_executable(echo_handler_test tests/echo_handler_test.cc)
add_executable(static_file_handler_test tests/static_file_handler_test.cc)
add_executable(handler_registry_test tests/handler_registry_test.cc)
add_executable(api_handler_test tests/api_handler_test.cc)
target_link_libraries(echo_handler_test gtest_main gmock_main handlers_lib request_lib response_lib handler_registry_lib)
target_link_libraries(static_file_handler_test gtest_main gmock_main mime_types_lib handlers_lib request_lib response_lib handler_registry_lib)
target_link_libraries(handler_registry_test gtest_main gmock_main handler_registry_lib handlers_lib)
target_link_libraries(api_handler_test gtest_main gmock_main mime_types_lib handlers_lib request_lib response_lib handler_registry_lib)

# New tests for request, response, and mime_types
add_executable(request_test tests/request_test.cc)
target_link_libraries(request_test request_lib gtest_main gmock_main)

add_executable(response_test tests/response_test.cc)
target_link_libraries(response_test response_lib gtest_main gmock_main)

add_executable(mime_types_test tests/mime_types_test.cc)
target_link_libraries(mime_types_test mime_types_lib gtest_main gmock_main)

# logger_tests
add_executable(logger_test tests/logger_test.cc)
target_link_libraries(logger_test logger_lib gtest_main gmock_main)

add_executable(logger_rotation_test tests/logger_rotation_test.cc)
target_link_libraries(logger_rotation_test logger_lib gtest_main gmock_main Boost::date_time)

# server configuration tests
add_executable(server_config_test tests/server_config_test.cc)
target_link_libraries(server_config_test server_config_lib gtest_main gmock_main)

# handler_server tests
add_executable(handler_server_test tests/handler_server_test.cc)
target_link_libraries(handler_server_test handler_server_lib server_lib session_lib gtest_main gmock_main)

# handler_dispatcher tests
add_executable(handler_dispatcher_test tests/handler_dispatcher_test.cc)
target_link_libraries(handler_dispatcher_test handler_dispatcher_lib handlers_lib request_lib gtest_main gmock_main)

# gtests
gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(echo_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(static_file_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(handler_registry_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(request_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(response_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(mime_types_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(logger_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(logger_rotation_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(server_config_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(handler_server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(handler_dispatcher_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(api_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Integration test
add_custom_target(integration_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_integration.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    DEPENDS server
    COMMENT "Running integration tests"
)

# Code coverage settings (also excluded in Docker)
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(
  TARGETS
    session_lib
    server_lib
    server
    config_parser
    request_lib
    response_lib
    mime_types_lib
    handlers_lib
    server_config_lib
    handler_server_lib
    handler_dispatcher_lib
    handler_registry_lib
    logger_lib
  TESTS
    config_parser_test
    server_test
    session_test 
    request_test
    response_test
    mime_types_test
    echo_handler_test
    static_file_handler_test
    handler_registry_test
    server_config_test
    handler_server_test
    handler_dispatcher_test
    logger_test
    logger_rotation_test
)
