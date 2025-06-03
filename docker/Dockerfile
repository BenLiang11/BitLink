### Build/test container ###
FROM i-am-steve:base AS builder

# Install required packages
RUN apt-get update && \
    apt-get install -y build-essential cmake libboost-all-dev \
    gcovr


# Set workdir and copy project
WORKDIR /usr/src/project
COPY . .

# Build the project
# RUN mkdir -p build && cd build && cmake .. && make && ctest --output-on_failure
RUN mkdir -p build && cd build && cmake .. && make

### Deploy container ###
FROM ubuntu:24.04 AS deploy

# Set working directory
WORKDIR /app

# Create logs directory for Boost.Log
RUN mkdir -p /app/logs

# Copy the built server binary
COPY --from=builder /usr/src/project/build/bin/server ./server

# Copy all config files into container
COPY config/ ./config/

# Create data directory and copy static content
RUN mkdir -p ./data
COPY data/static1 ./data/static1
COPY data/static2 ./data/static2

# Expose port 80 (just for reference, not mandatory)
EXPOSE 80

# Run the server with config
CMD ["./server", "./config/complete_server_config.conf"]
