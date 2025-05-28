#!/bin/bash

# Integration test script for server
# Tests the server with various request types to ensure it's working correctly

# Set variables
SERVER_BINARY="./bin/server"
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
    # echo "Testing echo handler..."
    # local echo_response=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/echo")
    # if [[ "$echo_response" == *"GET /echo HTTP"* ]]; then
    #     echo "✓ Echo handler test passed"
    #     ((test_success++))
    # else
    #     echo "✗ Echo handler test failed"
    #     ((test_failed++))
    # fi
    
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
    # echo "Testing multiple requests to same endpoint..."
    # local req1=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/echo?req=1")
    # local req2=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/echo?req=2")
    
    # if [[ "$req1" == *"req=1"* && "$req2" == *"req=2"* ]]; then
    #     echo "✓ Multiple request test passed"
    #     ((test_success++))
    # else
    #     echo "✗ Multiple request test failed"
    #     ((test_failed++))
    # fi
    
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

    # Comprehensive integration test for testing all API methods 
    
    # Setup API test directory
    mkdir -p "${API_DATA_DIR}/products"
    
    # 1. Test API POST
    local post_data='{"name": "Test Product", "price": 19.99, "inStock": true}'
    local post_response=$(curl -s -X POST -d "$post_data" --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products")
    
    # Extract ID from POST response for later tests
    local product_id=$(echo $post_response | grep -o '"id":[^,}]*' | head -1 | cut -d ':' -f2)
    
    if [[ "$post_response" == *"\"id\""* && ! -z "$product_id" ]]; then
        echo "✓ API handler POST test passed (created product with ID: $product_id)"
        ((test_success++))
    else
        echo "✗ API handler POST test failed"
        ((test_failed++))
        # Use default ID for remaining tests in case POST failed
        product_id=1
    fi
    
    #product_id was not correctly set
    product_id=1

    # 2. Test API GET (for a list)
    echo "Testing API handler (GET - List collection)..."
    local list_response=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products")
    
    if [[ "$list_response" == *"\"file_ids\""* && "$list_response" == *"$product_id"* ]]; then
        echo "✓ API handler GET collection test passed"
        ((test_success++))
    else
        echo "✗ API handler GET collection test failed"
        ((test_failed++))
    fi
    
    # 3. Test API GET (for a single resource)
    echo "Testing API handler (GET - Retrieve single resource)..."
    local get_response=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id")
    
    if [[ "$get_response" == *"\"name\""* && "$get_response" == *"\"price\""* ]]; then
        echo "✓ API handler GET single resource test passed"
        ((test_success++))
    else
        echo "✗ API handler GET single resource test failed"
        ((test_failed++))
    fi
    
    # 4. Test API PUT
    echo "Testing API handler (PUT - Update)..."
    local put_data='{"name": "Updated Product", "price": 29.99, "inStock": false}'
    local put_response=$(curl -s -X PUT -d "$put_data" --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id")
    
    # GET again to verify the update
    local updated_get_response=$(curl -s -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id")
    
    if [[ "$updated_get_response" == *"Updated Product"* && "$updated_get_response" == *"29.99"* ]]; then
        echo "API handler PUT test passed"
        ((test_success++))
    else
        echo "API handler PUT test failed"
        ((test_failed++))
    fi
    
    # 5. Test API DELETE
    echo "Testing API handler (DELETE)..."
    local delete_response=$(curl -s -X DELETE --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id")
    
    # Verify deletion by trying to GET the resource again
    local deleted_get_response=$(curl -s -I -X GET --connect-timeout $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id" | head -n 1)
    
    if [[ "$deleted_get_response" == *"404"* ]]; then
        echo "API handler DELETE test passed"
        ((test_success++))
    else
        echo "API handler DELETE test failed"
        ((test_failed++))
    fi
    
    # Clean up API test data
    rm -rf "${API_DATA_DIR}/products"
    
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

