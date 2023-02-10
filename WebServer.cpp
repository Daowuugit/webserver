#include "WebServer.h"

WebServer::WebServer() {

}

WebServer::~WebServer() {

}

void WebServer::run(){
    const char* ip = "127.0.0.1";
    int port = 5555, ret;

    struct sockaddr_in address;
    bzero(&address, sizeof address);
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof address);
    assert(ret != -1);

    ret = listen(listenfd, 8);
    assert(ret != -1);

}

void WebServer::epoll_addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    

}