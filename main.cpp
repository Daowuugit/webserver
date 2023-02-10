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

#include "ThreadPool.h"
#include "http.h"

#define MAX_EVENT_NUMBER 1024

int setnonblocking(int fd); // 文件描述符设置为非阻塞
void epoll_addfd(int epollfd, int fd, bool oneshot); // epoll边沿触发
void epoll_modfd(int epollfd, int fd);

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
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof address);
    assert(ret != -1);
    ret = listen(listenfd, 8);
    assert(ret != -1);
    int epollfd = epoll_create(8);
    assert(epollfd >= 0);
    epoll_addfd(epollfd, listenfd, false);
    std::vector<epoll_event> events(MAX_EVENT_NUMBER); 
    http* https = new http[1024];

    while(true) {
        ret = epoll_wait(epollfd, &events[0], MAX_EVENT_NUMBER, -1);
        assert(ret != -1);
        for(int i = 0; i < ret; ++ i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof client_address;
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                assert(connfd >= 0);
                epoll_addfd(epollfd, connfd, true);
            }
            else if (events[i].events & EPOLLIN) {
                char buffer[1024];
                while((ret = recv(sockfd, buffer, sizeof(buffer)-1, 0)) > 0) {
                    buffer[ret] = '\0';
                    https[sockfd].append_buffer(buffer);
                }
                epoll_modfd(epollfd, sockfd);
            }
            else if (events[i].events & EPOLLOUT) {
                https[sockfd].send_buffer(sockfd);
                epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
                close(sockfd);
            }
            else {
                std::cout << "error\n";        
            }
        }

    }
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

void epoll_modfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}