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
#include <sys/stat.h>
#include <fcntl.h> 
#include <assert.h>
#include <atomic>
#include <sys/uio.h>

class http{
public:
    enum PARSE_STATE { // 请求报文状态
        REQUEST_LINE,  // 请求行
        HEADERS,       // 头部
        BODY,          // 信息体
        FINISH,        // 完成
    };

    void init(int connfd, struct sockaddr_in connaddr); // 初始化连接对象
    void Close(); // 断开连接
    void task();  // 封装线程池可接受任务

    void parse_request();   // 解析http请求 
    void parse_body(std::string body);  // 解析消息主体
    int ConverHex(char ch); // 16进制转10进制
    void respond_request(); // 响应http请求
    std::string get_file_type(std::string file);
    
    static std::string srcDir; // 根目录
    static std::atomic<int> userCount; // 用户个数
private:
    int sockfd;
    struct sockaddr_in addr;
    std::vector<char> buffer;
    std::unordered_map<std::string, std::string> mp;
};

#endif // HTTP_H