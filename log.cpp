#include "log.h"

Log* Log::Instance() {
    static Log p;
    return &p;
}

Log::Log(){
    path = "../log";
    suffix = ".log";
    fp = nullptr;
}

void Log::init(int capatity) {
    this -> capatity = capatity;
    isClosed = false;
    Log* p = Instance();
    std::unique_ptr<std::thread> new_thread(new std::thread(log_thread));
    one_thread = std::move(new_thread);
    fp = nullptr;
    setfp();
}

void Log::Fflush() {
    if (fp) {
        fflush(fp);
    }
}

void Log::setfp() {
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    today = t.tm_mday;
    char fileName[1024] = {0};
    snprintf(fileName, 1023, "%s/%04d_%02d_%02d%s", 
            path.data(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix.data());
    if (fp) {
        fflush(fp);
        fclose(fp);
    }
    fp = fopen(fileName, "a+");

    // std::cout << str << std::endl;
    // if (fp) std::cout << fputs("1234", fp) << std::endl;
    // else std::cout << "false" << std::endl;

}

Log::~Log() {
    isClosed = true;
    cv_consumer.notify_all();
    cv_producer.notify_all();
    if (one_thread->joinable()) {
        one_thread -> join();
    }
    if (fp) {
        fflush(fp);
        fclose(fp);
        fp = nullptr;
    }
    std::cout << "log" << std::endl;
}

void Log::log_thread() {
    Instance() -> consumer();
}

void Log::consumer() {
    while(true) {
        std::unique_lock<std::mutex> lock(m_mutex);
        while(que.empty()) {
            if (isClosed) {
                return ;
            }
            cv_consumer.wait(lock);
        }
        std::string str = que.front();
        que.pop();
        lock.unlock();
        std::cout << str;
        fputs(str.data(), fp);
        fflush(fp);
        // fputs("hello", fp);
        cv_producer.notify_one();
    }
}

void Log::producer(std::string str) {
    std::unique_lock<std::mutex> lock(m_mutex);
    while(que.size() >= capatity) {
        cv_producer.wait(lock);
    }
    que.push(str);
    lock.unlock();
    cv_consumer.notify_one();
}

void Log::add_log(int level, const char *format, ...) {
    std::string ans;
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    if (today != t.tm_mday) {
        setfp();
    }

    {
        char buffer[1024] = {0};
        int n = snprintf(buffer, 1023, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        std::string str;
        switch(level) {
        case 1:
            str += "[debug]: ";
            break;
        case 2:
            str += "[info]: ";
            break;
        case 3:
            str += "[warn]: ";
            break;
        case 4:
            str += "[error]: ";
            break;
        }

        va_start(vaList, format);
        int m = vsnprintf(buffer+n, 1023-n, format, vaList);
        va_end(vaList);
        for(int i = 0; i < n+m; ++ i) str += buffer[i];
        str += "\n\0";
        producer(str);
        fputs("hello", fp);
    }
    
}
