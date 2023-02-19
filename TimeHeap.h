#ifndef TIMEHEAP_H
#define TIMEHEAP_H

#include <vector>
#include <memory>
#include <time.h>
#include <chrono>
#include <functional>
#include <iostream>

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;


struct node{
    int sockfd;
    TimeStamp expires; // 关闭时间
    std::function<void()> cb;
    bool operator<(const node &b) {
        return expires < b.expires;
    }
};

class TimeHeap{
public:
    TimeHeap();
    ~TimeHeap();

    void tick();
    void add_node(int fd, int timeout, std::function<void()> cb);   
private:
    void init();
    void clear();
    void Swap(int id_a, int id_b);
    void up(int id);
    void down(int id);
    void pop_top();

    int size;
    std::vector<struct node> heap;
};

#endif // TIMEHEAP_H