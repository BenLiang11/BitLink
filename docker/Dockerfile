### Build/test container ###
# Define builder stage
FROM ubuntu:24.04 AS builder

# Install required packages
RUN apt-get update && \
    apt-get install -y build-essential cmake libboost-all-dev

# Copy project and set working directory
COPY . /usr/src/project
WORKDIR /usr/src/project

# Create build directory and run build
RUN mkdir -p build && cd build && cmake .. && make && ctest --output-on_failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:24.04 AS deploy

# Copy the built binary
COPY --from=builder /usr/src/project/build/bin/server .

# Expose port
EXPOSE 80

# Set server run command
ENTRYPOINT ["./server"]
CMD ["80"]
