#include "hvnetpp/Channel.h"
#include "hvnetpp/EventLoop.h"
#include <sys/epoll.h>

namespace hvnetpp {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      eventHandling_(false),
      addedToLoop_(false),
      tied_(false) {
}

Channel::~Channel() {
}

void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

void Channel::update() {
    if (!addedToLoop_ && isNoneEvent()) {
        return;
    }
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::handleEvent() {
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (!guard) {
            return;
        }
    }
    handleEventWithGuard();
}

void Channel::handleEventWithGuard() {
    eventHandling_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & (EPOLLERR)) {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) readCallback_();
    }
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}

void Channel::remove() {
    if (!addedToLoop_) {
        return;
    }
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

} // namespace hvnetpp
