#include "TimeHeap.h"

TimeHeap::TimeHeap() {
    init();
}

TimeHeap::~TimeHeap() {
    clear();
}

void TimeHeap::init() {
    heap.clear();
    size = 0;
}

void TimeHeap::clear() {
    heap.clear();
    size = 0;
}

void TimeHeap::Swap(int id_a, int id_b) {
    std::swap(heap[id_a], heap[id_b]);
}

void TimeHeap::up(int id) {
    while(id) {
        int fa_id = (id+1)/2-1;
        if (heap[id] < heap[fa_id]) {
            Swap(id, fa_id);
            id = fa_id;
        }
        else break;
    }
}

void TimeHeap::down(int id) {
    while(id < size) {
        int j = (id+1)*2-1;
        if (j < size) {
            if (j+1 < size && heap[j+1] < heap[j]) j ++;
            if (heap[j] < heap[id]) {
                Swap(id, j);
                id = j;
            }
            else break;
        }
        else break;
    }   
}

void TimeHeap::add_node(int fd, int timeout, std::function<void()> cb){
    heap.push_back({fd, Clock::now() + MS(timeout), cb});
    size ++;
    up(size-1);
}

void TimeHeap::pop_top() {
    Swap(0, size-1);
    size --;
    heap.pop_back();
    down(0);
}

void TimeHeap::tick() {
    while(!heap.empty()) {
        if(std::chrono::duration_cast<MS>(heap[0].expires - Clock::now()).count() > 0) { 
            break; 
        }
        heap[0].cb();
        pop_top();
    }
}