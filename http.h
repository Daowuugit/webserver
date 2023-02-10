#ifndef HTTP_H
#define HTTP_H

#include <vector>
#include <cstring>
#include <sys/socket.h>

class http{
public:
    void append_buffer(char *rd) {
        for(int i = 0; i < strlen(rd); ++ i) {
            buffer.push_back(rd[i]);
        }
    }
    
    void send_buffer(int fd) {
        send(fd, &buffer[0], buffer.size(), 0);
    }
private:
    std::vector<char> buffer;
};

#endif // HTTP_H