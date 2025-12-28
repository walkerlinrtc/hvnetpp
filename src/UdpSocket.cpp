#include "hvnetpp/UdpSocket.h"
#include "hvnetpp/EventLoop.h"
#include "hvnetpp/Channel.h"
#include "hvnetpp/Buffer.h"
#include "hvnetpp/SocketsOps.h"
#include "RTCLog.h"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace hvnetpp {

UdpSocket::UdpSocket(EventLoop* loop, const std::string& name)
    : loop_(loop),
      name_(name),
      sockfd_(sockets::createNonblockingOrDie(AF_INET)), // Default to IPv4 for now
      channel_(new Channel(loop, sockfd_)),
      readBuf_(65536) { // Max UDP packet size
    
    sockets::setReuseAddr(sockfd_, true);
    sockets::setReusePort(sockfd_, true);
    
    channel_->setReadCallback(std::bind(&UdpSocket::handleRead, this));
}

UdpSocket::~UdpSocket() {
    channel_->disableAll();
    channel_->remove();
    sockets::close(sockfd_);
}

bool UdpSocket::bind(const InetAddress& addr) {
    sockets::bindOrDie(sockfd_, addr.getSockAddr());
    channel_->enableReading();
    return true;
}

ssize_t UdpSocket::sendTo(const void* data, size_t len, const InetAddress& destAddr) {
    return ::sendto(sockfd_, data, len, 0, destAddr.getSockAddr(), sizeof(struct sockaddr_in6));
}

ssize_t UdpSocket::sendTo(Buffer* buf, const InetAddress& destAddr) {
    return sendTo(buf->peek(), buf->readableBytes(), destAddr);
}

void UdpSocket::handleRead() {
    loop_->assertInLoopThread();
    struct sockaddr_in6 peerAddr;
    socklen_t addrLen = sizeof peerAddr;
    ssize_t n = ::recvfrom(sockfd_, readBuf_.data(), readBuf_.size(), 0, 
                           sockets::sockaddr_cast(&peerAddr), &addrLen);
    
    if (n >= 0) {
        if (readCallback_) {
            Buffer buf;
            buf.append(readBuf_.data(), n);
            InetAddress peer(peerAddr);
            readCallback_(peer, &buf);
        }
    } else {
        RTCLOG(RTC_ERROR, "UdpSocket::handleRead() error: %s", strerror(errno));
    }
}

} // namespace hvnetpp
