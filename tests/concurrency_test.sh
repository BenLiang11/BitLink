#!/usr/bin/env bash
set -euo pipefail

# Integration test script for concurrency tests

PORT="" # Will be dynamically found and assigned
SERVER_BIN="./bin/server"
CFG_FILE="../config/complete_server_config.conf"
SLEEP_PATH="/sleep?dur=2000"
HEALTH_PATH="/health"
MAX_HEALTH_MS=500 # /health must finish < 0.5 s
LOG_DIR="./logs"; mkdir -p "$LOG_DIR"
ORIGINAL_PORT=""

echo "=== Concurrency Test Debug Information ==="
echo "Server binary: $SERVER_BIN"
echo "Config file: $CFG_FILE"
echo "Log directory: $LOG_DIR"
echo "Max health response time: ${MAX_HEALTH_MS}ms"
echo "Sleep endpoint duration: 2000ms"
echo ""

# Function to find an available port
find_available_port() {
  local start_port=8000
  local end_port=9000
  
  echo "Finding available port in range $start_port-$end_port..."
  
  for ((port=start_port; port<=end_port; port++)); do
    # Check if port is available by trying to bind to it
    if ! netstat -ln 2>/dev/null | grep -q ":$port " && \
       ! lsof -i ":$port" >/dev/null 2>&1; then
      # Double-check with a quick connection attempt
      if ! timeout 1 bash -c "echo >/dev/tcp/localhost/$port" 2>/dev/null; then
        echo "Found available port: $port"
        PORT="$port"
        return 0
      fi
    fi
  done
  
  echo "ERROR: No available port found in range $start_port-$end_port"
  exit 1
}

# Cleanup function to restore config file
cleanup() {
  local exit_code=$?
  echo ""
  echo "=== Cleanup Process ==="
  
  # Stop server if it's running
  if [[ -n "${SERVER_PID:-}" ]]; then
    stop_server
  fi
  
  # Restore original config file
  restore_config
  
  # Dump server logs for debugging
  echo ""
  echo "=== Server Logs for Debugging ==="
  if [[ -f "$LOG_DIR/server.log" ]]; then
    echo "Server log contents:"
    echo "===================="
    cat "$LOG_DIR/server.log"
    echo "===================="
  else
    echo "No server log file found at $LOG_DIR/server.log"
  fi
  
  echo "Cleanup completed"
  exit $exit_code
}

# Set up trap to ensure cleanup happens on script exit
trap cleanup EXIT INT TERM

modify_config() {
  echo "=== Config File Management ==="
  
  # Check if config file exists
  if [[ ! -f "$CFG_FILE" ]]; then
    echo "ERROR: Config file not found at $CFG_FILE"
    exit 1
  fi
  
  # Extract original port
  ORIGINAL_PORT=$(grep -E '^listen[[:space:]]+[0-9]+;' "$CFG_FILE" | grep -oE '[0-9]+' || echo "")
  if [[ -z "$ORIGINAL_PORT" ]]; then
    echo "ERROR: Could not find listen directive in config file"
    exit 1
  fi
  
  echo "Original port detected: $ORIGINAL_PORT"
  
  # Find an available port for testing
  find_available_port
  
  # Modify config to use the available port
  echo "Modifying config to use port $PORT for testing..."
  sed -i.tmp "s/^listen[[:space:]]*[0-9]*;/listen $PORT;/" "$CFG_FILE"
  rm -f "${CFG_FILE}.tmp" 2>/dev/null || true
  
  echo "Config file modified to use port $PORT"
  echo "New listen directive: $(grep -E '^listen[[:space:]]+[0-9]+;' "$CFG_FILE")"
}

restore_config() {
  if [[ -n "$ORIGINAL_PORT" ]]; then
    echo "Restoring original config file to use port $ORIGINAL_PORT..."
    if sed -i.tmp "s/^listen[[:space:]]*[0-9]*;/listen $ORIGINAL_PORT;/" "$CFG_FILE" 2>/dev/null; then
      rm -f "${CFG_FILE}.tmp" 2>/dev/null || true
      echo "Config file restored with original port: $ORIGINAL_PORT"
    else
      echo "Warning: Failed to restore original config file"
    fi
  else
    echo "Warning: Original port not known, config file may not be restored"
  fi
}

# Check if server binary exists
if [[ ! -f "$SERVER_BIN" ]]; then
  echo "ERROR: Server binary not found at $SERVER_BIN"
  exit 1
fi

# Modify config file
modify_config

start_server() {
  echo ""
  echo "=== Starting Server ==="
  echo "Starting server on port $PORT..."
  echo "Command: $SERVER_BIN $CFG_FILE"
  
  "$SERVER_BIN" "$CFG_FILE" >"$LOG_DIR/server.log" 2>&1 &
  SERVER_PID=$!
  echo "Server PID: $SERVER_PID"
  
  # Give server a moment to start
  echo "Waiting for server to initialize..."
  sleep 1
  
  echo "Testing server connectivity on port $PORT..."
  
  for attempt in {1..20}; do
    echo "Health check attempt $attempt/20..."
    if curl -s --connect-timeout 2 --max-time 5 "http://localhost:$PORT$HEALTH_PATH" >/dev/null 2>&1; then
      echo "Server is ready on port $PORT"
      echo "Server startup successful after $attempt attempts"
      return 0
    fi
    echo "  Attempt $attempt failed, retrying in 0.1s..."
    sleep 0.1
  done
  
  echo "ERROR: Server failed to start or become ready"
  echo "Server logs:"
  echo "============"
  cat "$LOG_DIR/server.log"
  echo "============"
  exit 1
}

stop_server() { 
  if [[ -n "${SERVER_PID:-}" ]]; then
    echo "Force-killing server (PID: $SERVER_PID)..."
    if kill -9 "$SERVER_PID" 2>/dev/null; then
      echo "Server force-killed successfully"
    else
      echo "Server process was already stopped or could not be terminated"
    fi
    # Brief wait to ensure process cleanup
    sleep 0.2
    SERVER_PID=""
  fi
}

measure_health() {
  local start_time=$(date +%s.%N)
  local response_time
  response_time=$(curl -s -o /dev/null -w '%{time_total}' "http://localhost:$PORT$HEALTH_PATH" 2>/dev/null || echo "ERROR")
  local end_time=$(date +%s.%N)
  local total_time=$(echo "$end_time - $start_time" | bc -l)
  
  echo "Health check timing:"
  echo "  curl reported: ${response_time}s"
  echo "  script measured: ${total_time}s"
  
  if [[ "$response_time" == "ERROR" ]]; then
    echo "ERROR: Health check failed"
    return 1
  fi
  
  echo "$response_time"
}

# Test execution
echo ""
echo "=== Starting Concurrency Test ==="
start_server

echo ""
echo "=== Initiating Sleep Request ==="
echo "Starting blocking sleep request (2000ms) in background..."
sleep_start_time=$(date +%s.%N)
curl -s "http://localhost:$PORT$SLEEP_PATH" >/dev/null & 
SLEEP_PID=$!
echo "Sleep request PID: $SLEEP_PID"

# Give the sleep request a moment to start
sleep 0.1

echo ""
echo "=== Testing Health Endpoint During Sleep ==="
echo "Measuring health endpoint response time while sleep request is active..."

HEALTH_TIME=$(measure_health)
if [[ "$?" -ne 0 ]]; then
  echo "ERROR: Health measurement failed"
  exit 1
fi

HEALTH_MS=$(printf "%.0f" "$(echo "$HEALTH_TIME*1000" | bc)")

echo ""
echo "=== Results ==="
echo "Health endpoint finished in ${HEALTH_MS}ms"
echo "Maximum allowed time: ${MAX_HEALTH_MS}ms"

# Check if sleep request is still running
if kill -0 "$SLEEP_PID" 2>/dev/null; then
  echo "Sleep request is still running (as expected)"
else
  echo "WARNING: Sleep request completed unexpectedly early"
fi

echo ""
if [ "$HEALTH_MS" -gt "$MAX_HEALTH_MS" ]; then
  echo "❌ CONCURRENCY TEST FAILED"
  echo "   Health endpoint took ${HEALTH_MS}ms, which exceeds the ${MAX_HEALTH_MS}ms limit"
  echo "   This suggests the server is not handling concurrent requests properly"
  exit 1
fi

echo "✅ CONCURRENCY TEST PASSED"
echo "   Health endpoint responded in ${HEALTH_MS}ms (under ${MAX_HEALTH_MS}ms limit)"
echo "   Server correctly handles concurrent requests"

echo ""
echo "=== Test Complete ==="
