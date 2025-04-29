cmake_minimum_required(VERSION 3.10.0)
project(i-am-steve)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMakeLists.txt에 Boost 경로 추가
include_directories(/opt/homebrew/include)  # Boost 헤더 파일 경로
link_directories(/opt/homebrew/lib)         # Boost 라이브러리 경로


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
find_package(Boost 1.50 REQUIRED COMPONENTS system log log_setup)
message(STATUS "Boost version: ${Boost_VERSION}")

# Project source files
include_directories(include)

# Server executable
<<<<<<< HEAD
add_executable(server src/main.cc src/session.cc src/server.cc src/config_parser.cc
                     src/request.cc src/response.cc src/mime_types.cc
                     src/handlers/echo_handler.cc src/handlers/static_file_handler.cc)
target_link_libraries(server Boost::system pthread)
=======
add_executable(server src/main.cc src/session.cc src/server.cc src/config_parser.cc src/logger.cc) #add logger.cc


target_link_libraries(server
    Boost::system
    Boost::log
    Boost::log_setup
)
>>>>>>> e2d4752 (Complete Boost.Log setup with rotation, timestamps, and server logging)

# config_parser related
add_library(config_parser src/config_parser.cc)
add_executable(config_parser_main src/config_parser_main.cc)
target_link_libraries(config_parser_main config_parser)

# server/session related
add_library(server_lib src/server.cc src/session.cc src/config_parser.cc
                      src/request.cc src/response.cc src/mime_types.cc
                      src/handlers/echo_handler.cc src/handlers/static_file_handler.cc)
add_library(session_lib src/session.cc src/request.cc src/response.cc
                       src/handlers/echo_handler.cc src/mime_types.cc)
add_library(handlers_lib src/handlers/echo_handler.cc src/handlers/static_file_handler.cc
                        src/mime_types.cc)

# request/response/mime_types related
add_library(request_lib src/request.cc)
add_library(response_lib src/response.cc)
add_library(mime_types_lib src/mime_types.cc)

# GoogleTest and test setup (excluded in Docker build)
# include(GoogleTest) // test can erase #
# add_subdirectory(/usr/src/googletest googletest)
#if(NOT DOCKER_BUILD) // test can erase #
  #include(GoogleTest) // test can erase #

  #Modified following line, unsure of purpose
  #add_subdirectory(third_party/googletest)
  #add_subdirectory(/usr/src/googletest googletest) 


  #add_executable(config_parser_test tests/config_parser_test.cc) // test can erase #
  #target_link_libraries(config_parser_test config_parser gtest_main) // test can erase #
  #gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)// test can erase #
#endif()// test can erase #

# server_test
#add_executable(server_test tests/server_test.cc) // test can erase #
#target_link_libraries(server_test server_lib gtest_main gmock_main) // test can erase #
#gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests) // test can erase #

# session_test
#add_executable(session_test tests/session_test.cc)// test can erase #
#target_link_libraries(session_test session_lib gtest_main gmock_main)// test can erase #
#gtest_discover_tests(session_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)// test can erase #



# handler_tests
add_executable(echo_handler_test tests/echo_handler_test.cc)
target_link_libraries(echo_handler_test gtest_main gmock_main handlers_lib request_lib response_lib)
gtest_discover_tests(echo_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_executable(static_file_handler_test tests/static_file_handler_test.cc)
target_link_libraries(static_file_handler_test gtest_main gmock_main mime_types_lib handlers_lib request_lib response_lib)
gtest_discover_tests(static_file_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# New tests for request, response, and mime_types
add_executable(request_test tests/request_test.cc)
target_link_libraries(request_test gtest_main gmock_main request_lib)
gtest_discover_tests(request_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_executable(response_test tests/response_test.cc)
target_link_libraries(response_test gtest_main gmock_main response_lib)
gtest_discover_tests(response_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_executable(mime_types_test tests/mime_types_test.cc)
target_link_libraries(mime_types_test gtest_main gmock_main mime_types_lib)
gtest_discover_tests(mime_types_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

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
  TESTS
    config_parser_test
    server_test
    session_test 
    request_test
    response_test
    mime_types_test
    echo_handler_test
    static_file_handler_test
)
