#!/bin/bash

# Integration test script for server
# Tests the server with various request types to ensure it's working correctly

# Set variables
SERVER_BINARY="./bin/server"
CONFIG_FILE="../config/complete_server_config.conf"
SERVER_HOST="localhost"
SERVER_PORT="" # Will be dynamically found and assigned
ORIGINAL_PORT=""
OUTPUT_DIR="./logs"
REQUEST_TIMEOUT=5  # Increased timeout for curl requests
SERVER_START_TIMEOUT=10  # Timeout for server startup
API_DATA_DIR="./data/api"

echo "=== Integration Test Debug Information ==="
echo "Server binary: $SERVER_BINARY"
echo "Config file: $CONFIG_FILE"
echo "Output directory: $OUTPUT_DIR"
echo "Request timeout: ${REQUEST_TIMEOUT}s"
echo "Server start timeout: ${SERVER_START_TIMEOUT}s"
echo ""

# Function to find an available port with timeout
find_available_port() {
  local start_port=8000
  local end_port=9000
  local max_attempts=100
  local attempt=0
  
  echo "Finding available port in range $start_port-$end_port..."
  
  for ((port=start_port; port<=end_port && attempt<max_attempts; port++, attempt++)); do
    # Multiple checks for port availability with timeouts
    local port_free=true
    
    # Check 1: netstat
    if timeout 2 netstat -ln 2>/dev/null | grep -q ":$port "; then
      port_free=false
    fi
    
    # Check 2: lsof
    if [[ "$port_free" == "true" ]] && timeout 2 lsof -i ":$port" >/dev/null 2>&1; then
      port_free=false
    fi
    
    # Check 3: direct connection attempt
    if [[ "$port_free" == "true" ]]; then
      if timeout 1 bash -c "echo >/dev/tcp/localhost/$port" 2>/dev/null; then
        port_free=false
      fi
    fi
    
    if [[ "$port_free" == "true" ]]; then
      echo "Found available port: $port"
      SERVER_PORT="$port"
      return 0
    fi
  done
  
  echo "ERROR: No available port found in range $start_port-$end_port after $max_attempts attempts"
  exit 1
}

# Cleanup function to restore config file
cleanup() {
  local exit_code=$?
  echo ""
  echo "=== Cleanup Process ==="
  
  # Stop server if it's running and we started it
  if [[ -n "${SERVER_PID:-}" && "${KILL_SERVER:-0}" -eq 1 ]]; then
    echo "Force-killing server (PID: $SERVER_PID)..."
    if kill -9 "$SERVER_PID" 2>/dev/null; then
      echo "Server force-killed successfully"
    else
      echo "Server process was already stopped or could not be terminated"
    fi
    # Brief wait to ensure process cleanup
    sleep 0.2
  fi
  
  # Restore original config file
  restore_config
  
  # Dump server logs for debugging
  echo ""
  echo "=== Server Logs for Debugging ==="
  if [[ -f "${OUTPUT_DIR}/server.log" ]]; then
    echo "Server log contents:"
    echo "===================="
    cat "${OUTPUT_DIR}/server.log"
    echo "===================="
  else
    echo "No server log file found at ${OUTPUT_DIR}/server.log"
  fi
  
  echo "Cleanup completed"
  exit $exit_code
}

# Set up trap to ensure cleanup happens on script exit
trap cleanup EXIT INT TERM

modify_config() {
  echo "=== Config File Management ==="
  
  # Check if config file exists
  if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "ERROR: Config file not found at $CONFIG_FILE"
    exit 1
  fi
  
  # Extract original port
  ORIGINAL_PORT=$(grep -E '^listen[[:space:]]+[0-9]+;' "$CONFIG_FILE" | grep -oE '[0-9]+' || echo "")
  if [[ -z "$ORIGINAL_PORT" ]]; then
    echo "ERROR: Could not find listen directive in config file"
    exit 1
  fi
  
  echo "Original port detected: $ORIGINAL_PORT"
  
  # Find an available port for testing
  find_available_port
  
  # Modify config to use the available port
  echo "Modifying config to use port $SERVER_PORT for testing..."
  if ! sed -i.tmp "s/^listen[[:space:]]*[0-9]*;/listen $SERVER_PORT;/" "$CONFIG_FILE"; then
    echo "ERROR: Failed to modify config file"
    exit 1
  fi
  rm -f "${CONFIG_FILE}.tmp" 2>/dev/null || true
  
  echo "Config file modified to use port $SERVER_PORT"
  echo "New listen directive: $(grep -E '^listen[[:space:]]+[0-9]+;' "$CONFIG_FILE")"
}

restore_config() {
  if [[ -n "$ORIGINAL_PORT" ]]; then
    echo "Restoring original config file to use port $ORIGINAL_PORT..."
    if sed -i.tmp "s/^listen[[:space:]]*[0-9]*;/listen $ORIGINAL_PORT;/" "$CONFIG_FILE" 2>/dev/null; then
      rm -f "${CONFIG_FILE}.tmp" 2>/dev/null || true
      echo "Config file restored with original port: $ORIGINAL_PORT"
    else
      echo "Warning: Failed to restore original config file"
    fi
  else
    echo "Warning: Original port not known, config file may not be restored"
  fi
}

# Check if server binary exists
if [[ ! -f "$SERVER_BINARY" ]]; then
  echo "ERROR: Server binary not found at $SERVER_BINARY"
  exit 1
fi

# Modify config file
modify_config

# Make sure output directory exists
mkdir -p ${OUTPUT_DIR}

# Function to check server status with timeout
check_server() {
    echo "Checking server connectivity on port $SERVER_PORT..."
    # Try to connect to server with explicit timeout
    if timeout $REQUEST_TIMEOUT curl -s --head --connect-timeout 2 --max-time 3 "http://${SERVER_HOST}:${SERVER_PORT}" >/dev/null 2>&1; then
        echo "Server is running on ${SERVER_HOST}:${SERVER_PORT}"
        return 0
    else
        echo "Server is not running or not reachable on port $SERVER_PORT"
        return 1
    fi
}

# Function to wait for server startup with timeout
wait_for_server() {
  local max_attempts=20
  local attempt=1
  
  echo "Waiting for server to start (max ${SERVER_START_TIMEOUT}s)..."
  
  while [[ $attempt -le $max_attempts ]]; do
    echo "  Server check attempt $attempt/$max_attempts..."
    
    # Check if server process is still running
    if [[ -n "${SERVER_PID:-}" ]] && ! kill -0 "$SERVER_PID" 2>/dev/null; then
      echo "ERROR: Server process died during startup"
      echo "Server logs:"
      echo "============"
      cat ${OUTPUT_DIR}/server.log 2>/dev/null || echo "No log file found"
      echo "============"
      return 1
    fi
    
    # Try to connect with timeout
    if timeout 3 bash -c "curl -s --head --connect-timeout 1 --max-time 2 'http://${SERVER_HOST}:${SERVER_PORT}' >/dev/null 2>&1"; then
      echo "Server is ready on port $SERVER_PORT (attempt $attempt)"
      return 0
    fi
    
    sleep 0.5
    ((attempt++))
  done
  
  echo "ERROR: Server failed to start within ${SERVER_START_TIMEOUT}s"
  return 1
}

# Function to run tests
run_tests() {
    echo ""
    echo "=== Running Integration Tests ==="
    local test_success=0
    local test_failed=0
    
    # Test static file handler (if data directory exists)
    echo "Testing static file handler..."
    local static_response=$(timeout $REQUEST_TIMEOUT curl -s -X GET --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/static1/index.html" 2>/dev/null || echo "TIMEOUT")
    if [[ "$static_response" == *"<html>"* && "$static_response" == *"</html>"* ]]; then
        echo "✓ Static file handler test passed"
        ((test_success++))
    else
        echo "✗ Static file handler test failed"
        echo "  Response: ${static_response:0:200}..." # Truncate long responses
        ((test_failed++))
    fi
    
    # Test 404 for non-existent path
    echo "Testing 404 response..."
    local not_found_response=$(timeout $REQUEST_TIMEOUT curl -s -I -X GET --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/nonexistent" 2>/dev/null | head -n 1 || echo "TIMEOUT")
    if [[ "$not_found_response" == *"404"* ]]; then
        echo "✓ 404 test passed"
        ((test_success++))
    else
        echo "✗ 404 test failed"
        echo "  Response: $not_found_response"
        echo "  Immediate server logs for debugging:"
        echo "  ====================================="
        if [[ -f "${OUTPUT_DIR}/server.log" ]]; then
            tail -n 20 "${OUTPUT_DIR}/server.log" | sed 's/^/  /'
        else
            echo "  No server log file found"
        fi
        echo "  ====================================="
        ((test_failed++))
    fi
    
    # Setup API test directory
    echo "Setting up API test directory..."
    mkdir -p "${API_DATA_DIR}/products"
    
    # Test API POST with timeout
    echo "Testing API handler (POST - Create)..."
    local post_data='{"name": "Test Product", "price": 19.99, "inStock": true}'
    local post_response=$(timeout $REQUEST_TIMEOUT curl -s -X POST -d "$post_data" --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products" 2>/dev/null || echo "TIMEOUT")
    
    local product_id=1 # Default fallback
    if [[ "$post_response" == *"\"id\""* && "$post_response" != "TIMEOUT" ]]; then
        local extracted_id=$(echo $post_response | grep -o '"id":[^,}]*' | head -1 | cut -d ':' -f2 | tr -d ' "')
        if [[ -n "$extracted_id" && "$extracted_id" =~ ^[0-9]+$ ]]; then
            product_id=$extracted_id
        fi
        echo "✓ API handler POST test passed (created product with ID: $product_id)"
        ((test_success++))
    else
        echo "✗ API handler POST test failed"
        echo "  Response: ${post_response:0:200}..."
        ((test_failed++))
    fi
    
    # Test API GET (for a list)
    echo "Testing API handler (GET - List collection)..."
    local list_response=$(timeout $REQUEST_TIMEOUT curl -s -X GET --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products" 2>/dev/null || echo "TIMEOUT")
    
    if [[ "$list_response" == *"\"file_ids\""* && "$list_response" != "TIMEOUT" ]]; then
        echo "✓ API handler GET collection test passed"
        ((test_success++))
    else
        echo "✗ API handler GET collection test failed"
        echo "  Response: ${list_response:0:200}..."
        ((test_failed++))
    fi
    
    # Test API GET (for a single resource)
    echo "Testing API handler (GET - Retrieve single resource)..."
    local get_response=$(timeout $REQUEST_TIMEOUT curl -s -X GET --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id" 2>/dev/null || echo "TIMEOUT")
    
    if [[ "$get_response" == *"\"name\""* && "$get_response" != "TIMEOUT" ]]; then
        echo "✓ API handler GET single resource test passed"
        ((test_success++))
    else
        echo "✗ API handler GET single resource test failed"
        echo "  Response: ${get_response:0:200}..."
        ((test_failed++))
    fi
    
    # Test API PUT
    echo "Testing API handler (PUT - Update)..."
    local put_data='{"name": "Updated Product", "price": 29.99, "inStock": false}'
    local put_response=$(timeout $REQUEST_TIMEOUT curl -s -X PUT -d "$put_data" --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id" 2>/dev/null || echo "TIMEOUT")
    
    # GET again to verify the update
    local updated_get_response=$(timeout $REQUEST_TIMEOUT curl -s -X GET --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id" 2>/dev/null || echo "TIMEOUT")
    
    if [[ "$updated_get_response" == *"Updated Product"* && "$updated_get_response" != "TIMEOUT" ]]; then
        echo "✓ API handler PUT test passed"
        ((test_success++))
    else
        echo "✗ API handler PUT test failed"
        echo "  Updated response: ${updated_get_response:0:200}..."
        ((test_failed++))
    fi
    
    # Test API DELETE
    echo "Testing API handler (DELETE)..."
    local delete_response=$(timeout $REQUEST_TIMEOUT curl -s -X DELETE --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id" 2>/dev/null || echo "TIMEOUT")
    
    # Verify deletion by trying to GET the resource again
    local deleted_get_response=$(timeout $REQUEST_TIMEOUT curl -s -I -X GET --connect-timeout 2 --max-time $REQUEST_TIMEOUT "http://${SERVER_HOST}:${SERVER_PORT}/api/products/$product_id" 2>/dev/null | head -n 1 || echo "TIMEOUT")
    
    if [[ "$deleted_get_response" == *"404"* && "$deleted_get_response" != "TIMEOUT" ]]; then
        echo "✓ API handler DELETE test passed"
        ((test_success++))
    else
        echo "✗ API handler DELETE test failed"
        echo "  Delete response: $deleted_get_response"
        ((test_failed++))
    fi
    
    # Clean up API test data
    echo "Cleaning up API test data..."
    rm -rf "${API_DATA_DIR}/products" 2>/dev/null || true
    
    # Output results
    echo ""
    echo "=== Test Results ==="
    echo "Tests passed: $test_success"
    echo "Tests failed: $test_failed"
    echo "----------------------------------------"
    
    if [ $test_failed -eq 0 ]; then
        echo "✅ All tests passed! The server is working correctly."
        return 0
    else
        echo "❌ Some tests failed. Check the output for details."
        return 1
    fi
}

echo ""
echo "=== Starting Integration Test ==="

# Start the server (if it's not already running)
check_server
if [ $? -ne 0 ]; then
    echo ""
    echo "=== Starting Server ==="
    echo "Starting server on port $SERVER_PORT..."
    echo "Command: ${SERVER_BINARY} ${CONFIG_FILE}"
    
    ${SERVER_BINARY} ${CONFIG_FILE} > ${OUTPUT_DIR}/server.log 2>&1 &
    SERVER_PID=$!
    echo "Server PID: $SERVER_PID"
    
    # Wait for server with timeout
    if ! wait_for_server; then
        echo "ERROR: Failed to start server"
        echo "Server logs:"
        echo "============"
        cat ${OUTPUT_DIR}/server.log 2>/dev/null || echo "No log file found"
        echo "============"
        exit 1
    fi
    
    # Flag to kill server when tests complete
    KILL_SERVER=1
else
    # Server already running
    echo "Server is already running on port $SERVER_PORT"
    KILL_SERVER=0
fi

# Run the tests
run_tests
TEST_RESULT=$?

echo ""
echo "=== Integration Test Complete ==="

# Note: Cleanup (including server shutdown and config restore) will be handled by the trap
exit $TEST_RESULT

