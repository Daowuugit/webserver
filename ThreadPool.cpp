#include "ThreadPool.h"

ThreadPool::ThreadPool(int ThreadCount):tasks(std::make_shared<ThreadPool::TaskQue>()) {
    assert(ThreadCount > 0);
    tasks->isClosed=false;
    while(ThreadCount --) {
        std::thread([tasks=tasks]{
            std::unique_lock<std::mutex> lock(tasks->m_mutex);
            while(true) {
                lock.lock();
                tasks->m_cv.wait(lock, [tasks]{return !tasks->task_que.empty() || tasks->isClosed;});
                if (tasks->isClosed) break;
                auto task_run = tasks->task_que.front();
                tasks->task_que.pop();
                lock.unlock();
                task_run();
            }
        }).detach();
    }
}

ThreadPool::~ThreadPool() {
    tasks->isClosed = true;
    tasks->m_cv.notify_all();
}

template<class F>
void ThreadPool::AddTask(F&& task) {
    {
        std::lock_guard<std::mutex> lock(tasks->m_mutex);
        tasks->task_que.emplace(std::forward(task));
    }
    tasks->m_cv.notify_one();
}