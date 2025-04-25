# docker/coverage.Dockerfile
FROM i-am-steve:base AS builder

WORKDIR /usr/src/project
COPY . .

# Build coverage report
RUN mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Coverage .. \
    && make coverage
