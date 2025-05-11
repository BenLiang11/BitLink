#!/bin/bash

# Integration test script for server
# Tests the server with various request types to ensure it's working correctly

# Set variables
SERVER_BINARY="../build/bin/webserver"
CONFIG_FILE="../config/server_config.conf"
SERVER_HOST="localhost"
SERVER_PORT="80"
OUTPUT_DIR="./logs"
REQUEST_TIMEOUT=3  # Timeout for curl requests in seconds

# Make sure output directory exists
mkdir -p ${OUTPUT_DIR}

# Function to check server status
check_server() {
    # Try to connect to server
    if curl -s --head --connect-timeout 2 http://${SERVER_HOST}:${SERVER_PORT} >/dev/null; then
        echo "Server is running on ${SERVER_HOST}:${SERVER_PORT}"
        return 0
    else
        echo "ERROR: Server is not running or not reachable"
        return 1
    fi
}

# Function to run tests
run_tests() {
    echo "Running integration tests..."
    local test_success=0
    local test_failed=0
    
    # Test echo handler
    echo "Testing echo handler..."
    local echo_response=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/echo")
    if [[ "$echo_response" == *"GET /echo HTTP"* ]]; then
        echo "✓ Echo handler test passed"
        ((test_success++))
    else
        echo "✗ Echo handler test failed"
        ((test_failed++))
    fi
    
    # Test static file handler (if data directory exists)
    echo "Testing static file handler..."
    local static_response=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/static1/index.html")
    if [[ "$static_response" == *"<html>"* && "$static_response" == *"</html>"* ]]; then
        echo "✓ Static file handler test passed"
        ((test_success++))
    else
        echo "✗ Static file handler test failed"
        ((test_failed++))
    fi
    
    # Test multiple requests to ensure per-request handler instantiation
    echo "Testing multiple requests to same endpoint..."
    local req1=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/echo?req=1")
    local req2=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/echo?req=2")
    
    if [[ "$req1" == *"req=1"* && "$req2" == *"req=2"* ]]; then
        echo "✓ Multiple request test passed"
        ((test_success++))
    else
        echo "✗ Multiple request test failed"
        ((test_failed++))
    fi
    
    # Test 404 for non-existent path
    echo "Testing 404 response..."
    local not_found_response=$(curl -s -I -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/nonexistent" | head -n 1)
    if [[ "$not_found_response" == *"404"* ]]; then
        echo "✓ 404 test passed"
        ((test_success++))
    else
        echo "✗ 404 test failed"
        ((test_failed++))
    fi
    
    # Output results
    echo "----------------------------------------"
    echo "Test Results: $test_success passed, $test_failed failed"
    
    if [ $test_failed -eq 0 ]; then
        echo "All tests passed! The server is working correctly."
        return 0
    else
        echo "Some tests failed. Check the output for details."
        return 1
    fi
}

# Start the server (if it's not already running)
check_server
if [ $? -ne 0 ]; then
    echo "Starting server..."
    ${SERVER_BINARY} ${CONFIG_FILE} > ${OUTPUT_DIR}/server.log 2>&1 &
    SERVER_PID=$!
    sleep 2  # Give the server time to start up
    
    # Check if server started successfully
    check_server
    if [ $? -ne 0 ]; then
        echo "Failed to start server. See log for details: ${OUTPUT_DIR}/server.log"
        exit 1
    fi
    
    # Flag to kill server when tests complete
    KILL_SERVER=1
else
    # Server already running
    KILL_SERVER=0
fi

# Run the tests
run_tests
TEST_RESULT=$?

# Clean up
if [ $KILL_SERVER -eq 1 ]; then
    echo "Stopping server (PID: $SERVER_PID)..."
    kill $SERVER_PID
fi

exit $TEST_RESULT

