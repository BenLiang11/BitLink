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

### Build & Test Instructions
```bash
mkdir build && cd build
cmake ..
make
```
Running the server
```
./bin/server ../config/server_config.conf

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

### Adding a New Request Handler
The server's functionality is extended by implementing new request handlers. Each handler is responsible for processing a specific type of request or handling requests for a particular URL prefix. Here's how to add a new handler:

1. Define the Handler Class:Create a new header file (e.g., `include/handlers/my_handler.h`) and source file (e.g., `src/handlers/my_handler.cc`).
```
#ifndef MY_HANDLER_H
#define MY_HANDLER_H

#include "request.h"
#include "response.h"
#include <memory>
#include <string>
#include <vector>

// MyHandler: Processes specific HTTP requests and generates appropriate responses
class MyHandler : public RequestHandler {
public:
    // Constructor with configuration parameters
    MyHandler(const std::string& param1, const std::string& param2);

    // Handle an HTTP request and generate a response
    std::unique_ptr<Response> handle_request(const Request& req) override;

    // Factory method for creating instances from configuration
    static std::unique_ptr<RequestHandler> Create(const std::vector<std::string>& args);

private:
    // Handler configuration parameters
    std::string param1_;
    std::string param2_;
    
    // Helper methods
    void helper_method();
};

#endif // MY_HANDLER_H
```

2. Implement the `handle_request` Method:
In your `.cc` file (`src/handlers/my_handler.cc`), implement the `handle_request` method. This method contains the core logic for your handler. It receives a `Request` object and must return a `Response` object. You will use the methods provided by the `Request` class to get information about the incoming request (method, URI, headers, body) and the methods of the `Response` class to set the status, headers, and body of the response.\
Refer to the StaticFileHandler::handle_request implementation below for an example of how to access request data, perform logic, and construct a response.

3. Implement the Static Create Factory Method:
Implement the static `Create` method in your `.cc` file. This method is responsible for parsing the string arguments provided from the configuration file and using them to construct an instance of your handler. It should perform necessary validation on the arguments and return a `std::unique_ptr<RequestHandler>` containing the newly created handler instance. If the arguments are invalid, it should throw a `std::invalid_argument exception`.\
Refer to the `StaticFileHandler::Create` implementation below for an example of validating the number and content of arguments before creating the handler object.

4. Register the Handler:
In your handler's .cc file (src/handlers/my_handler.cc), add a static registration call outside of any function. This uses a static initializer to register your handler's Create function with the central HandlerRegistry under a specific string name. This is how the server knows about your handler type and can create instances based on the configuration file.
```
#include "handler_registry.h"
// ...
namespace {
    // This static variable's initialization calls RegisterHandler.
    // "MyHandlerName" is the name you will use in the server configuration file.
    const bool my_handler_registered = HandlerRegistry::RegisterHandler("MyHandlerName", MyHandler::Create);
}
```
5. Update `CMakeLists.txt`: Edit the main `CMakeLists.txt` file at the project root. Find the add_library(handlers_lib ...) command and add your new handler source file (`src/handlers/my_handler.cc`) to the list of source files for this library.

6. Configure the Server: Edit the server configuration file (`config/server_config.conf` or your custom file). In the section defining the handlers for a specific port, add an entry that maps a URL path prefix to your new handler, using the string name you registered in step 4 ("MyHandlerName") and providing the necessary string arguments that your handler's Create method expects.

### Example: Static File Handler
This handler serves static files from a configured root directory, mapping parts of the request URI to file paths. It includes path sanitization to prevent directory traversal attacks.
```
#include "handlers/static_file_handler.h"
#include "handler_registry.h"
#include "mime_types.h"
#include "request.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

StaticFileHandler::StaticFileHandler(const std::string& serving_path, const std::string& root_directory)
    : serving_path_(serving_path), root_directory_(root_directory) {
    if (serving_path_.empty() || serving_path_[0] != '/') {
        throw std::invalid_argument("StaticFileHandler serving_path must start with '/'. Path: " + serving_path_);
    }
    if (serving_path_.length() > 1 && serving_path_.back() == '/') {
        serving_path_.pop_back();
    }
}

std::unique_ptr<Response> StaticFileHandler::handle_request(const Request& req) {
    auto response = std::make_unique<Response>();

    if (req.method() != "GET") {
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>405 Method Not Allowed</h1></body></html>");
        return response;
    }

    std::string request_uri = req.uri();

    if (request_uri.rfind(serving_path_, 0) != 0) {
        response->set_status(Response::INTERNAL_SERVER_ERROR);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>500 Internal Server Error</h1><p>URI mismatch.</p></body></html>");
        return response;
    }

    std::string relative_path;
    if (request_uri.length() == serving_path_.length() || (serving_path_ == "/" && request_uri.length() == 1)) {
        relative_path = "index.html";
    } else if (serving_path_ == "/") {
        relative_path = request_uri.substr(1);
    } else {
        relative_path = request_uri.substr(serving_path_.length() + 1);
    }
    
    if (relative_path.empty() || relative_path.back() == '/') {
        relative_path += "index.html";
    }

    fs::path fs_root_path = root_directory_;
    fs::path fs_relative_path = relative_path;
    fs::path full_file_path = fs_root_path / fs_relative_path;

    std::string canonical_root = fs::weakly_canonical(fs_root_path).string();
    std::string canonical_file = fs::weakly_canonical(full_file_path).string();

    if (canonical_file.rfind(canonical_root, 0) != 0) {
        response->set_status(Response::FORBIDDEN);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>403 Forbidden</h1><p>Requested resource is outside the allowed directory.</p></body></html>");
        return response;
    }
    
    std::ifstream file(full_file_path.string(), std::ios::binary);
    if (!file) {
        response->set_status(Response::NOT_FOUND);
        response->set_header("Content-Type", "text/html");
        response->set_body("<html><body><h1>404 Not Found</h1><p>The requested file could not be found.</p></body></html>");
        return response;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    response->set_status(Response::OK);
    response->set_header("Content-Type", get_mime_type(full_file_path.string()));
    response->set_body(content);

    return response;
}

std::string StaticFileHandler::get_mime_type(const std::string& file_path) const {
    size_t dot_pos = file_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string extension = file_path.substr(dot_pos + 1);
        return MimeTypes::GetMimeType(extension);
    }
    return "application/octet-stream";
}

std::unique_ptr<RequestHandler> StaticFileHandler::Create(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        throw std::invalid_argument("StaticFileHandler factory requires 2 arguments: serving_path and root_directory. Got " + std::to_string(args.size()));
    }
    
    // Check for empty arguments
    if (args[0].empty()) {
        throw std::invalid_argument("StaticFileHandler serving_path cannot be empty");
    }
    if (args[1].empty()) {
        throw std::invalid_argument("StaticFileHandler root_directory cannot be empty");
    }
    
    return std::make_unique<StaticFileHandler>(args[0], args[1]);
}

namespace {
    const bool static_file_handler_registered = HandlerRegistry::RegisterHandler("StaticHandler", StaticFileHandler::Create);
} 
```
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
