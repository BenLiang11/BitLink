
#!/bin/bash
# Sample script to test the server with multiple requests
./bin/server config/test.conf &
SERVER_PID=$!
sleep 2
# sending 3 times
curl -s http://localhost:8080
curl -s http://localhost:8080
curl -s http://localhost:8080
sleep 1
# sutting down the server
kill $SERVER_PID