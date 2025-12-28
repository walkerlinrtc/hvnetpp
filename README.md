# hvnetpp

A lightweight C++ network framework based on the Reactor pattern.

## Introduction

hvnetpp is a high-performance, non-blocking network library for C++. It uses `epoll` for event notification and provides a simple API for building TCP servers and UDP applications. It is designed to be easy to understand and use, making it suitable for learning network programming or building small to medium-sized network applications on Linux.

## Features

- **Non-blocking I/O**: Based on the Reactor pattern using `epoll` (Linux only).
- **TCP Support**: Easy-to-use `TcpServer` and `TcpConnection` classes for handling TCP connections.
- **UDP Support**: wrappers for UDP socket operations.
- **Timers**: Efficient timer management via `TimerQueue`.
- **Callbacks**: Modern C++11 callbacks (`std::function`) for connection establishment, message reception, and write completion.
- **Logging**: Integrated logging via `rtclog`.

## Requirements

- **Operating System**: Linux (requires `sys/epoll.h`).
- **Compiler**: C++11 compliant compiler (e.g., GCC, Clang).
- **Build System**: CMake 3.10 or higher.

## Build Instructions

1.  Clone the repository:
    ```bash
    git clone https://github.com/walkerlinrtc/hvnetpp.git
    cd hvnetpp
    ```

2.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

3.  Run CMake and make:
    ```bash
    cmake ..
    make
    ```

4.  Run the test server (if built):
    ```bash
    ./test_build
    ```

## Usage Example

Below is a simple example of a TCP Server that logs connections and messages:

```cpp
#include "hvnetpp/EventLoop.h"
#include "hvnetpp/TcpServer.h"
#include "rtclog.h"
#include <iostream>

int main() {
    // Initialize logger
    rtclog_init("TestServer");
    rtclog_set_level(RTC_INFO);

    hvnetpp::EventLoop loop;
    hvnetpp::InetAddress addr(9999); // Listen on port 9999

    hvnetpp::TcpServer server(&loop, addr, "TestServer");

    // Set connection callback
    server.setConnectionCallback([](const hvnetpp::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            RTCLOG(RTC_INFO, "Client connected: %s", conn->peerAddress().toIpPort().c_str());
        } else {
            RTCLOG(RTC_INFO, "Client disconnected: %s", conn->peerAddress().toIpPort().c_str());
        }
    });

    // Set message callback
    server.setMessageCallback([](const hvnetpp::TcpConnectionPtr& conn, hvnetpp::Buffer* buf) {
        std::string msg = buf->retrieveAllAsString();
        RTCLOG(RTC_INFO, "Received %d bytes: %s", msg.size(), msg.c_str());
        conn->send(msg); // Echo back
    });

    server.start();
    loop.loop(); // Start the event loop

    return 0;
}
```

## Directory Structure

- `include/hvnetpp/`: Public header files.
- `src/`: Source code implementation.
  - `internal/`: Internal helpers (e.g., CircularBuffer).
  - `thirdparty/`: Third-party libraries (e.g., rtclog).
- `test_build.cpp`: Example usage file.
