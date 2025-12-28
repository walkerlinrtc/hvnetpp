#pragma once

#include "hvnetpp/TcpConnection.h"
#include <map>
#include <string>

namespace hvnetpp {
class EventLoop;
class Acceptor; // Helper for accept()
class InetAddress;

class TcpServer {
public:
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg);
    ~TcpServer();

    void start();
    
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;
    const std::string ipPort_;
    const std::string name_;
    
    std::unique_ptr<Acceptor> acceptor_; // Internal class to handle bind/listen/accept
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    
    int nextConnId_;
    ConnectionMap connections_;
};

} // namespace hvnetpp
