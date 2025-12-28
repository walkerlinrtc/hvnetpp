#pragma once

#include <vector>
#include <map>

struct epoll_event;

namespace hvnetpp {

class Channel;
class EventLoop;

class Poller {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    ~Poller();

    void poll(int timeoutMs, ChannelList* activeChannels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    bool hasChannel(Channel* channel) const;

private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    using EventList = std::vector<struct epoll_event>;

    EventLoop* loop_;
    int epollfd_;
    EventList events_;
    std::map<int, Channel*> channels_;
};

} // namespace hvnetpp
