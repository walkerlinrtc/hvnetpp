#include "hvnetpp/EventLoop.h"
#include "hvnetpp/Channel.h"
#include "hvnetpp/Poller.h"
#include "hvnetpp/TimerQueue.h"
#include "rtclog.h"

#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

using namespace hvnetpp;

namespace {
__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        RTCLOG(RTC_FATAL, "Failed in eventfd");
        abort();
    }
    return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe {
public:
    IgnoreSigPipe() {
        ::signal(SIGPIPE, SIG_IGN);
    }
};
IgnoreSigPipe initObj;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      threadId_(std::this_thread::get_id()),
      poller_(new Poller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr),
      pendingQueue_(new PendingQueue(16))
{
    RTCLOG(RTC_DEBUG, "EventLoop created %p in thread %zu", this, std::hash<std::thread::id>{}(threadId_));
    if (t_loopInThisThread) {
        RTCLOG(RTC_FATAL, "Another EventLoop %p exists in this thread %zu", t_loopInThisThread, std::hash<std::thread::id>{}(threadId_));
    } else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    RTCLOG(RTC_DEBUG, "EventLoop %p of thread %zu destructs in thread %zu", this, std::hash<std::thread::id>{}(threadId_), std::hash<std::thread::id>{}(std::this_thread::get_id()));
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    RTCLOG(RTC_TRACE, "EventLoop %p start looping", this);

    while (!quit_) {
        activeChannels_.clear();
        poller_->poll(kPollTimeMs, &activeChannels_);
        
        eventHandling_ = true;
        for (Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            channel->handleEvent(Timestamp(std::chrono::steady_clock::now()));
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;

        doPendingFunctors();
    }

    RTCLOG(RTC_TRACE, "EventLoop %p stop looping", this);
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        uint64_t one = 1;
        ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::updateChannel(Channel* channel) {
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assertInLoopThread();
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) {
        RTCLOG(RTC_FATAL, "EventLoop::assertInLoopThread - Created in thread %zu current thread %zu", std::hash<std::thread::id>{}(threadId_), std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        RTCLOG(RTC_ERROR, "EventLoop::handleRead() reads %zd bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors() {
    callingPendingFunctors_ = true;
    
    while (true) {
        PendingQueue::Node* node = pendingQueue_->peek();
        if (!node) {
            break;
        }
        
        FunctorTask& task = node->data;
        if (task.functorPtr) {
            (*task.functorPtr)();
            delete task.functorPtr;
        }
        
        pendingQueue_->consume(node);
    }

    callingPendingFunctors_ = false;
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb) {
    Timestamp time(std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int64_t>(delay * 1000)));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb) {
    Timestamp time(std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int64_t>(interval * 1000)));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId) {
    return timerQueue_->cancel(timerId);
}

void EventLoop::queueInLoop(Functor cb) {
    Functor* f = new Functor(std::move(cb));
    PendingQueue::Node* node = pendingQueue_->reserve();
    if (node) {
        node->data.functorPtr = f;
        pendingQueue_->commit(node, 1);
        
        if (!isInLoopThread() || callingPendingFunctors_) {
            uint64_t one = 1;
            ssize_t n = ::write(wakeupFd_, &one, sizeof one);
        }
    } else {
        // Queue full or error
        delete f;
        RTCLOG(RTC_ERROR, "queueInLoop failed: queue full");
    }
}
