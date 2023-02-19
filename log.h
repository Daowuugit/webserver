#ifndef LOG_H
#define LOG_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <iostream>
#include <thread>
#include <memory>
#include <sys/time.h>
#include <stdarg.h>

class Log {
public:
    static Log* Instance();
    void init(int capatity = 1000);
    void add_log(int, int count,...);
    void Fflush();
    ~Log();

private:
    Log();
    static void log_thread();
    void consumer();
    void producer(std::string);
    void setfp();

private:
    int capatity;
    bool isClosed;
    std::condition_variable cv_consumer;
    std::condition_variable cv_producer;
    std::mutex m_mutex;
    std::queue<std::string> que;
    std::unique_ptr<std::thread> one_thread;
    std::string path, suffix;
    FILE* fp;
    int today;
};

#define DEBUG_LOG(count, ...) Log::Instance() -> add_log(1, count, ##__VA_ARGS__)
#define INFO_LOG(count, ...) Log::Instance() -> add_log(2, count, ##__VA_ARGS__)
#define WARN_LOG(count, ...) Log::Instance() -> add_log(3, count, ##__VA_ARGS__)
#define ERROR_LOG(count, ...) Log::Instance() -> add_log(4, count, ##__VA_ARGS__)

#endif // LOG_H