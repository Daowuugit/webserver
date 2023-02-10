#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h> // 
#include <arpa/inet.h> // inet_pton() htons()  
#include <cstring> // bzero()
#include <cassert> // assert()
#include <sys/epoll.h> //   

class WebServer {
public:
    WebServer();
    ~WebServer();

    void run(); // 运行服务器

    void epoll_addfd(int epollfd, int fd);

private:

};


#endif //WEBSERVER_H
