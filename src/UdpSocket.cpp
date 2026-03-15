#include "hvnetpp/UdpSocket.h"
#include "hvnetpp/EventLoop.h"
#include "hvnetpp/Channel.h"
#include "hvnetpp/Buffer.h"
#include "hvnetpp/SocketsOps.h"
#include "rtclog.h"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace hvnetpp {

namespace {

socklen_t socketAddrLength(const InetAddress& addr) {
    return static_cast<socklen_t>(
        addr.family() == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in));
}

} // namespace

UdpSocket::UdpSocket(EventLoop* loop, const std::string& name)
    : loop_(loop),
      name_(name),
      sockfd_(sockets::createNonblockingUdpOrDie(AF_INET)),
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
    return ::sendto(sockfd_, data, len, 0, destAddr.getSockAddr(), socketAddrLength(destAddr));
}

ssize_t UdpSocket::sendTo(Buffer* buf, const InetAddress& destAddr) {
    return sendTo(buf->peek(), buf->readableBytes(), destAddr);
}

void UdpSocket::handleRead() {
    loop_->assertInLoopThread();
    struct sockaddr_storage peerAddrStorage;
    socklen_t addrLen = sizeof peerAddrStorage;
    ssize_t n = ::recvfrom(sockfd_, readBuf_.data(), readBuf_.size(), 0, 
                           reinterpret_cast<struct sockaddr*>(&peerAddrStorage), &addrLen);
    
    if (n >= 0) {
        if (readCallback_) {
            Buffer buf;
            buf.append(readBuf_.data(), n);
            if (peerAddrStorage.ss_family == AF_INET6) {
                const struct sockaddr_in6* peerAddr =
                    reinterpret_cast<const struct sockaddr_in6*>(&peerAddrStorage);
                InetAddress peer(*peerAddr);
                readCallback_(peer, &buf);
            } else {
                const struct sockaddr_in* peerAddr =
                    reinterpret_cast<const struct sockaddr_in*>(&peerAddrStorage);
                InetAddress peer(*peerAddr);
                readCallback_(peer, &buf);
            }
        }
    } else {
        RTCLOG(RTC_ERROR, "UdpSocket::handleRead() error: %s", strerror(errno));
    }
}

} // namespace hvnetpp
