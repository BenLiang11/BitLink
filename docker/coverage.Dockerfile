# docker/coverage.Dockerfile
FROM i-am-steve:base AS builder

COPY . .

# Build coverage report
RUN mkdir build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Coverage .. \
    && make

# Generate coverage report
RUN cd build \
    && make coverage      \
