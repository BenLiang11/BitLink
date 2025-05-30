#!/usr/bin/env bash
set -euo pipefail

# Integration test script for concurrency tests

PORT=8080 # Default Port so choose any empty port for testing
SERVER_BIN="./bin/server"
CFG_FILE="../config/server_config.conf"
SLEEP_PATH="/sleep?dur=2000"
HEALTH_PATH="/health"
MAX_HEALTH_MS=500 # /health must finish < 0.5 s
LOG_DIR="./logs"; mkdir -p "$LOG_DIR"

start_server() {
  "$SERVER_BIN" "$CFG_FILE" >"$LOG_DIR/server.log" 2>&1 &
  SERVER_PID=$!
  for _ in {1..20}; do
    curl -s "http://localhost:$PORT$HEALTH_PATH" >/dev/null && return
    sleep 0.1
  done
  echo "Server failed to bind"; cat "$LOG_DIR/server.log"; kill $SERVER_PID; exit 1
}

stop_server() { kill "$SERVER_PID"; wait "$SERVER_PID" 2>/dev/null || true; }

measure_health() {
  curl -s -o /dev/null -w '%{time_total}' "http://localhost:$PORT$HEALTH_PATH"
}

# Test
start_server
curl -s "http://localhost:$PORT$SLEEP_PATH" >/dev/null & # blocking request

HEALTH_TIME=$(measure_health)
HEALTH_MS=$(printf "%.0f" "$(echo "$HEALTH_TIME*1000" | bc)")

echo "health finished in ${HEALTH_MS} ms"
if [ "$HEALTH_MS" -gt "$MAX_HEALTH_MS" ]; then
  echo "Concurrency test failed"; stop_server; exit 1
fi
echo "Concurrency test passed"; stop_server
