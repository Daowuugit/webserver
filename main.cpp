#include <sys/socket.h> // 
#include <arpa/inet.h> // inet_pton() htons()  
#include <cstring> // bzero()
#include <cassert> // assert()
#include <sys/epoll.h> //  
#include <fcntl.h> 
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <functional>
#include <signal.h>
#include <memory>

#include "ThreadPool.h"
#include "http.h"
#include "TimeHeap.h"

#define MAX_EVENT_NUMBER 1024
#define MAX_FD_COUNT 65536
#define alarm_second 5
#define one_second 1000

static int pipefd[2];

int setnonblocking(int fd); // 文件描述符设置为非阻塞
void epoll_addfd(int epollfd, int fd, bool oneshot); // epoll边沿触发
void sig_handler(int sig); // 信号回调函数
void addsig(int sig); // 设置信号回调函数
void cb_timeout(http *client); // 时间超限函数

int main() {
    const char* ip = "127.0.0.1";
    int port = 5555, ret;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); // 端口复用
    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    // inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof address);
    assert(ret != -1);
    ret = listen(listenfd, 16);
    assert(ret != -1);
    printf("listening ...\n");
    int epollfd = epoll_create(32);
    assert(epollfd >= 0);
    epoll_addfd(epollfd, listenfd, false);
    
    std::unique_ptr<ThreadPool> pool(new ThreadPool());
    std::vector<epoll_event> events(MAX_EVENT_NUMBER); 
    http* https = new http[MAX_FD_COUNT];

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
    assert(ret != -1);
    setnonblocking(pipefd[1]);
    epoll_addfd(epollfd, pipefd[0], false);
    addsig(SIGALRM);
    bool timeout = false;
    std::unique_ptr<TimeHeap> time_heap(new TimeHeap);
    alarm(alarm_second);

    while(true) {
        ret = epoll_wait(epollfd, &events[0], MAX_EVENT_NUMBER, -1);
        assert(ret != -1 || ret == -1 && errno == EINTR);
        for(int i = 0; i < ret; ++ i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof client_address;
                int connfd;
                while((connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength)) >= 0) {
                    https[connfd].init(connfd, client_address, epollfd);
                    epoll_addfd(epollfd, connfd, true);
                    time_heap->add_node(connfd, 60*one_second, std::bind(cb_timeout, &https[connfd]));
                }
                if (errno == EAGAIN) {
                    continue;
                }
                assert(connfd != -1);
            }
            else if (sockfd == pipefd[0]) {
                int sig;
                char signals[1024] = {0};
                ret = recv(pipefd[0], signals, sizeof(signals)-1, 0);
                if (ret > 0) {
                    for(int i = 0; i < ret; ++ i) {
                        switch(signals[i]) {
                        case SIGALRM:
                            timeout = true;
                            // std::cout << "sigalrm" << std::endl;
                            break;
                        }
                    }
                }
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
            }
            else if (events[i].events & EPOLLIN) {
                pool->pool_add_task(std::bind(&http::task, &https[sockfd]));
            }
            else if (events[i].events & EPOLLOUT) {
                pool->pool_add_task(std::bind(&http::write_, &https[sockfd]));
            }
            else {
                std::cout << "error\n";        
            }
        }
        if (timeout) {
            timeout = false;
            time_heap -> tick();
            alarm(alarm_second);
        }
    }
    close(epollfd);
    close(listenfd);
    delete [] https;
}

int setnonblocking(int fd) { 
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void epoll_addfd(int epollfd, int fd, bool oneshot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void sig_handler(int sig) {
    int save_errno = errno;
    int msg  = sig;
    send(pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

void addsig(int sig) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void cb_timeout(http* client) {
    client -> Close();
    // std::cout << "timeout" << std::endl;
}
