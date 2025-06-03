# docker/coverage.Dockerfile
FROM i-am-steve:base AS builder

WORKDIR /usr/src/project
COPY . .

# Build coverage report and run tests
# RUN mkdir -p build && cd build \
#     && cmake -DCMAKE_BUILD_TYPE=Coverage .. \
#     && make coverage && ctest --rerun-failed --output-on-failure

RUN mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Coverage ..

WORKDIR /usr/src/project/build

RUN mkdir -p ./data
COPY data/static1 ./data/static1
COPY data/static2 ./data/static2
RUN mkdir -p ./data/uploads

# Give permissions to the data directory so that sqlite can write to it
RUN chmod 777 ./data

RUN make coverage