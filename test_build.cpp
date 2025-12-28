#include "hvnetpp/EventLoop.h"
#include "hvnetpp/TcpServer.h"
#include "RTCLog.h"
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

    hvnetpp::EventLoop loop;
    hvnetpp::InetAddress addr(9999);

    hvnetpp::TcpServer server(&loop, addr, "TestServer");
    server.setConnectionCallback([](const hvnetpp::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            RTCLOG(RTC_INFO, "Client connected: %s", conn->peerAddress().toIpPort().c_str());
        } else {
            RTCLOG(RTC_INFO, "Client disconnected: %s", conn->peerAddress().toIpPort().c_str());
        }
    });

    server.start();
    RTCLOG(RTC_INFO, "Server started loop");
    loop.loop();
    return 0;
}
