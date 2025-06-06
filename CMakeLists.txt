cmake_minimum_required(VERSION 3.10.0)
project(i-am-steve)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# GoogleTest with Homebrew
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Boost include and lib dirs
include_directories(/opt/homebrew/include) 
link_directories(/opt/homebrew/lib)

# Prevent in-source builds to avoid clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# Enable debug mode by default when building inside a dev shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output compiled binaries to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable CMake testing
include(CTest)
enable_testing()

# Boost settings
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system log log_setup date_time)
message(STATUS "Boost version: ${Boost_VERSION}")

# Third-party libraries - compiled from source
# SQLite3 amalgamation library
add_library(sqlite3_static STATIC
    third_party/sqlite3/sqlite3.c
)

target_include_directories(sqlite3_static PUBLIC 
    ${CMAKE_SOURCE_DIR}/third_party/sqlite3
)

# Compile with recommended SQLite3 options for production
target_compile_definitions(sqlite3_static PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_RTREE
    SQLITE_ENABLE_JSON1
    SQLITE_ENABLE_STAT4
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    SQLITE_LIKE_DOESNT_MATCH_BLOBS
    SQLITE_MAX_EXPR_DEPTH=0
    SQLITE_OMIT_DECLTYPE
    SQLITE_OMIT_DEPRECATED
    SQLITE_USE_ALLOCA
)

# libqrencode library
add_library(qrencode_static STATIC
    third_party/libqrencode/bitstream.c
    third_party/libqrencode/mask.c
    third_party/libqrencode/mmask.c
    third_party/libqrencode/mqrspec.c
    third_party/libqrencode/qrencode.c
    third_party/libqrencode/qrinput.c
    third_party/libqrencode/qrspec.c
    third_party/libqrencode/rsecc.c
    third_party/libqrencode/split.c
)

target_include_directories(qrencode_static PUBLIC 
    ${CMAKE_SOURCE_DIR}/third_party/libqrencode
)

target_compile_definitions(qrencode_static PRIVATE
    MAJOR_VERSION=4
    MINOR_VERSION=1
    MICRO_VERSION=1
    VERSION="4.1.1"
    STATIC_IN_RELEASE=static
    HAVE_LIBPTHREAD=1
    HAVE_STRDUP=1
    HAVE_SDL=0
)

# Set proper library properties
set_target_properties(sqlite3_static PROPERTIES
    OUTPUT_NAME sqlite3
    POSITION_INDEPENDENT_CODE ON
)

set_target_properties(qrencode_static PROPERTIES
    OUTPUT_NAME qrencode
    POSITION_INDEPENDENT_CODE ON
)

# Server executable
add_executable(server src/main.cc)
target_include_directories(server PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(server server_lib handler_registry_lib logger_lib thread_generator_lib)

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

add_library(file_system_lib src/real_file_system.cc)
target_include_directories(file_system_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(fake_file_system_lib src/fake_file_system.cc)
target_include_directories(fake_file_system_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# SlugGenerator library for generating URL-safe codes
add_library(slug_generator_lib src/utils/slug_generator.cc)
target_include_directories(slug_generator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# QR code generator library
add_library(qr_generator_lib src/utils/qr_generator.cc)
target_include_directories(qr_generator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(qr_generator_lib PRIVATE ${CMAKE_SOURCE_DIR}/third_party/libqrencode)
target_link_libraries(qr_generator_lib qrencode_static)

# File validator library
add_library(file_validator_lib src/utils/file_validator.cc)
target_include_directories(file_validator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# URL validator library
add_library(url_validator_lib src/utils/url_validator.cc)
target_include_directories(url_validator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# HTTP parser library for form data and multipart parsing
add_library(http_parser_lib src/utils/http_parser.cc)
target_include_directories(http_parser_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# JSON builder library for creating API responses
add_library(json_builder_lib src/utils/json_builder.cc)
target_include_directories(json_builder_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Thread Generator library
add_library(thread_generator_lib src/utils/thread_generator.cc)
target_include_directories(thread_generator_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# URL Shortening Service library
add_library(url_shortening_service_lib src/services/url_shortening_service.cc)
target_include_directories(url_shortening_service_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(url_shortening_service_lib 
                      url_validator_lib 
                      slug_generator_lib 
                      qr_generator_lib
                      handlers_lib)

# File Upload Service library
add_library(file_upload_service_lib src/services/file_upload_service.cc)
target_include_directories(file_upload_service_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(file_upload_service_lib 
                      file_validator_lib 
                      slug_generator_lib 
                      qr_generator_lib
                      handlers_lib)

# Registry library
add_library(handler_registry_lib src/handler_registry.cc)
target_include_directories(handler_registry_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Handlers library (✔ includes health_handler and sleep_handler for health check)
add_library(handlers_lib 
            src/handlers/echo_handler.cc 
            src/handlers/static_file_handler.cc
            src/handlers/not_found_handler.cc
            src/handlers/api_handler.cc
            src/handlers/health_handler.cc
            src/handlers/url_shortener_handler.cc
            # Database implementation
            src/database/sqlite_database.cc
            # Models
            src/models/link_data.cc
            )

target_include_directories(handlers_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(handlers_lib 
                      mime_types_lib
                      request_lib 
                      response_lib
                      handler_registry_lib
                      file_system_lib 
                      fake_file_system_lib
                      url_shortening_service_lib
                      file_upload_service_lib
                      http_parser_lib
                      json_builder_lib
                      qrencode_static
                      sqlite3_static
                    )

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

# GoogleTest and test setup
include(GoogleTest)
if(NOT DOCKER_BUILD)
  include(GoogleTest)
  #add_subdirectory(/usr/src/googletest googletest)

add_executable(config_parser_test tests/config_parser_test.cc)
target_link_libraries(config_parser_test config_parser GTest::gtest_main)
gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

endif()
# Additional unit tests
add_executable(server_test tests/server_test.cc)
target_link_libraries(server_test server_lib GTest::gtest_main GTest::gmock_main)

add_executable(session_test tests/session_test.cc)
target_link_libraries(session_test session_lib GTest::gtest_main GTest::gmock_main)

# Link Data and SQLite Database tests
add_executable(link_data_test tests/link_data_test.cc)
target_link_libraries(link_data_test handlers_lib GTest::gtest_main GTest::gmock_main)

add_executable(sqlite_database_test tests/sqlite_database_test.cc)
target_link_libraries(sqlite_database_test handlers_lib GTest::gtest_main GTest::gmock_main)

add_executable(slug_generator_test tests/slug_generator_test.cc)
target_link_libraries(slug_generator_test slug_generator_lib GTest::gtest_main GTest::gmock_main)

add_executable(qr_generator_test tests/qr_generator_test.cc)
target_link_libraries(qr_generator_test qr_generator_lib GTest::gtest_main GTest::gmock_main)

add_executable(file_validator_test tests/file_validator_test.cc)
target_link_libraries(file_validator_test file_validator_lib GTest::gtest_main GTest::gmock_main)

add_executable(url_validator_test tests/url_validator_test.cc)
target_link_libraries(url_validator_test url_validator_lib GTest::gtest_main GTest::gmock_main)

add_executable(http_parser_test tests/http_parser_test.cc)
target_link_libraries(http_parser_test http_parser_lib GTest::gtest_main GTest::gmock_main)

add_executable(json_builder_test tests/json_builder_test.cc)
target_link_libraries(json_builder_test json_builder_lib GTest::gtest_main GTest::gmock_main)

add_executable(url_shortening_service_test tests/url_shortening_service_test.cc)
target_link_libraries(url_shortening_service_test url_shortening_service_lib handlers_lib GTest::gtest_main GTest::gmock_main)

add_executable(file_upload_service_test tests/file_upload_service_test.cc)
target_link_libraries(file_upload_service_test file_upload_service_lib handlers_lib GTest::gtest_main GTest::gmock_main)

add_executable(url_shortener_handler_test tests/url_shortener_handler_test.cc)
target_link_libraries(url_shortener_handler_test handlers_lib request_lib response_lib handler_registry_lib GTest::gtest_main GTest::gmock_main)

add_executable(thread_generator_test tests/thread_generator_test.cc)
target_link_libraries(thread_generator_test
    thread_generator_lib
    GTest::gtest_main
    GTest::gmock_main
    Boost::log
    Boost::log_setup
    Boost::date_time
    pthread
)

add_executable(echo_handler_test tests/echo_handler_test.cc)
add_executable(static_file_handler_test tests/static_file_handler_test.cc)
add_executable(handler_registry_test tests/handler_registry_test.cc)
add_executable(api_handler_test tests/api_handler_test.cc)
target_link_libraries(echo_handler_test GTest::gtest_main GTest::gmock_main handlers_lib request_lib response_lib handler_registry_lib)
target_link_libraries(static_file_handler_test GTest::gtest_main GTest::gmock_main mime_types_lib handlers_lib request_lib response_lib handler_registry_lib)
target_link_libraries(handler_registry_test GTest::gtest_main GTest::gmock_main handler_registry_lib handlers_lib)
target_link_libraries(api_handler_test GTest::gtest_main GTest::gmock_main mime_types_lib handlers_lib request_lib response_lib handler_registry_lib)

add_executable(request_test tests/request_test.cc)
target_link_libraries(request_test request_lib GTest::gtest_main GTest::gmock_main)
add_executable(response_test tests/response_test.cc)
target_link_libraries(response_test response_lib GTest::gtest_main GTest::gmock_main)
add_executable(mime_types_test tests/mime_types_test.cc)
target_link_libraries(mime_types_test mime_types_lib GTest::gtest_main GTest::gmock_main)

add_executable(logger_test tests/logger_test.cc)
target_link_libraries(logger_test logger_lib GTest::gtest_main GTest::gmock_main)
add_executable(logger_rotation_test tests/logger_rotation_test.cc)
target_link_libraries(logger_rotation_test logger_lib GTest::gtest_main GTest::gmock_main Boost::date_time)

add_executable(server_config_test tests/server_config_test.cc)
target_link_libraries(server_config_test server_config_lib GTest::gtest_main GTest::gmock_main)

add_executable(handler_server_test tests/handler_server_test.cc)
target_link_libraries(handler_server_test handler_server_lib server_lib session_lib GTest::gtest_main GTest::gmock_main)

add_executable(handler_dispatcher_test tests/handler_dispatcher_test.cc)
target_link_libraries(handler_dispatcher_test handler_dispatcher_lib handlers_lib request_lib GTest::gtest_main GTest::gmock_main)

add_executable(health_handler_test tests/health_handler_test.cc)

target_link_libraries(health_handler_test handlers_lib request_lib response_lib GTest::gtest_main GTest::gmock_main)

# Discover all tests for CTest
gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(link_data_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(sqlite_database_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(slug_generator_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(qr_generator_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(file_validator_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(url_validator_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(http_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(json_builder_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(url_shortening_service_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(file_upload_service_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(url_shortener_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
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
gtest_discover_tests(health_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(thread_generator_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Integration test script
add_custom_target(integration_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_integration.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    DEPENDS server
    COMMENT "Running integration tests"
)

# Code coverage settings (excluded in Docker)
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
    slug_generator_lib
    qr_generator_lib
    file_validator_lib
    url_validator_lib
    http_parser_lib
    json_builder_lib
    url_shortening_service_lib
    file_upload_service_lib
    thread_generator_lib
  TESTS
    config_parser_test
    server_test
    session_test
    link_data_test
    sqlite_database_test
    slug_generator_test
    qr_generator_test
    file_validator_test
    url_validator_test
    http_parser_test
    json_builder_test
    url_shortening_service_test
    file_upload_service_test
    url_shortener_handler_test
    request_test
    response_test
    mime_types_test
    echo_handler_test
    static_file_handler_test
    api_handler_test
    handler_registry_test
    server_config_test
    handler_server_test
    handler_dispatcher_test
    logger_test
    logger_rotation_test
    health_handler_test
    thread_generator_test
)

add_test(NAME IntegrationTest COMMAND bash ../tests/test_integration.sh)

# TEMPORARY TESTING
add_test(
 NAME ConcurrencyTest
 COMMAND bash ../tests/concurrency_test.sh
)