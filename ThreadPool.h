#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <cassert>
#include <thread>

class ThreadPool{
public:
    ThreadPool(int threadCount = 8): taskQue(std::make_shared<TaskQue>()){
        assert(threadCount > 0);
        taskQue->isClosed = false;
        std::thread([taskQue = taskQue]{
            while(true) {
                std::unique_lock<std::mutex> lock(taskQue->m_mutex);
                taskQue->m_cv.wait(lock, [taskQue]{return !taskQue->que.empty() || taskQue->isClosed;});
                if (taskQue->isClosed) break;
                auto run = taskQue->que.front();
                taskQue->que.pop();
                lock.unlock();
                run();
            }
        }).detach();
    };

    ~ThreadPool() {
        taskQue->isClosed = true;
        taskQue->m_cv.notify_all();
    }

    template<class F>
    void pool_add_task(F&& task) {
        {
            std::lock_guard<std::mutex> lock(taskQue->m_mutex);
            taskQue->que.emplace(std::forward<F>(task));
        }
        taskQue->m_cv.notify_one();
    }


private:
    struct TaskQue{
        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<std::function<void()>> que;
        bool isClosed;
    };
    std::shared_ptr<TaskQue> taskQue;
};


#endif //THREADPOOL_H
