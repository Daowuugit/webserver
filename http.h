#ifndef HTTP_H
#define HTTP_H

#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <regex>
#include <unordered_map>


class http{
public:
    enum PARSE_STATE { // 语法分析状态
        REQUEST_LINE,  // 请求行
        HEADERS,       // 头部
        BODY,          // 信息体
        FINISH,        // 完成
    };

    void init(int connfd, struct sockaddr_in connaddr); // 初始化连接对象
    void close(); // 断开连接
    void task();  // 封装线程池可接受任务

    void read();  // 接收http请求
    void write(); // 发送http请求
    void parse_request();   // 解析http请求 
    void respond_request(); // 响应http请求
    
    
private:
    int sockfd;
    struct sockaddr_in addr;
    std::vector<char> buffer;
    std::unordered_map<std::string, std::string> mp;
};

#endif // HTTP_H