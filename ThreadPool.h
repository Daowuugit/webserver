#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <memory> // shared_ptr
#include <cassert>
#include <thread>

class ThreadPool {
public:
    explicit ThreadPool(int ThreadCount = 4);
    ~ThreadPool();

    template<class F>
    void AddTask(F&&);

private:
    struct TaskQue{
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool isClosed;
        std::queue<std::function<void()>> task_que;
    };
    std::shared_ptr<TaskQue> tasks;
};


#endif //THREADPOOL_H
