# I-AM-STEVE

I-AM-STEVE is a modular and extensible web server written in modern C++17. It supports dynamic request routing, file serving, configurable logging, and handler-based request processing.

## Prerequisites

- CMake ≥ 3.10
- C++17-compatible compiler (e.g. `g++`, `clang++`)
- Make build tool
- Boost (Log, Log_Setup, Date_Time, System)
- Docker (for builds/tests)

## Project Structure
```
I-AM-STEVE/
├── build/                         # CMake build output directory
├── cmake/                         # CMake helper scripts
├── config/                        # Server configuration files
├── data/                          # Static data (HTML, txt, etc.)
├── docker/                        # Docker-related files
├── include/                       # Public header files
│   ├── handlers/                  # Request handler headers
├── src/                           # Source files
│   ├── handlers/                  # Request handler implementations
├── tests/                         # Unit and integration tests
├── CMakeLists.txt                 # Main CMake build script
```

## Getting Started

### Build Instructions
```bash
mkdir build && cd build
cmake ..
make
```
Running the server
```
./bin/server ../../config/server_config.conf

# Example test: logger_test
./bin/logger_test
```
An example
We can also create coverage reports with the following commands
```
mkdir build_coverage
cd build_coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```
Download the HTML files in `/build_coverage/report/` to view full coverage report
### Docker
Running docker images
```
# Build using the provided Docker files
docker build -f docker/base.Dockerfile -t i-am-steve:base .
docker build -f docker/Dockerfile -t i-am-steve:latest .

# Run the container
docker run -p 8080:8080 -v /path/to/config:/app/config i-am-steve:latest
```

Submitting docker configuration to cloud
```
gcloud builds submit --config docker/cloudbuild.yaml
```
