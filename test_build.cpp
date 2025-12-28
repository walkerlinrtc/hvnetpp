#include "zlnetpp/EventLoop.h"
#include "zlnetpp/TcpServer.h"
#include "rtclog.h"
#include <iostream>

int main() {
    // Initialize logger
    rtclog_init("TestServer");
    rtclog_configure("logs/xrtc.log", true);
    rtclog_set_level(RTC_DEBUG);

    RTCLOG(RTC_INFO, "Application started");

    RTCLOG(RTC_WARN, "This is a warning message");
    RTCLOG(RTC_ERROR, "This is an error message with code: %d", 404);

    RTCLOG(RTC_INFO, "TestServer starting...");

    zlnetpp::EventLoop loop;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    zlnetpp::TcpServer server(&loop, addr, "TestServer");
    server.setConnectionCallback([](const zlnetpp::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            RTCLOG(RTC_INFO, "Client connected: %s", conn->peerAddress().toIpPort().c_str());
        } else {
            RTCLOG(RTC_INFO, "Client disconnected: %s", conn->peerAddress().toIpPort().c_str());
        }
    });

    server.start();
    RTCLOG(RTC_INFO, "Server started loop");
    // loop.loop(); // Don't block for test
    return 0;
}
