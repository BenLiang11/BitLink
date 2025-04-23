#!/bin/bash

# Start the server in the background
./bin/server config/test.conf &
SERVER_PID=$!
sleep 1  # Give it time to start

# Make a request
curl -s http://localhost:8080 > actual_output.txt

# Define expected output
echo -e "HTTP/1.1 200 OK\n\nHello, World!" > expected_output.txt  # Customize based on your actual response

# Compare actual and expected
diff actual_output.txt expected_output.txt

# Capture diff result
RESULT=$?

# Clean up
kill $SERVER_PID

# Exit with success or failure
exit $RESULT

