#include "http.h"

using namespace std;

void http::init(int connfd, struct sockaddr_in connaddr) {
    sockfd = connfd;
    addr = connaddr;
    cout << "get client" << endl;
}

void http::close() {
    buffer.clear();
    cout << "discard client" << endl;
}

void http::task() {

}

void http::read() {
    char temp[1024] = {0};
    int len;
    while((len = recv(sockfd, temp, 1023, 0)) > 0) {
        for(int i = 0; i < len; ++ i) buffer.push_back(temp[i]);
    }
    cout << "accept message" << endl;
    parse_request();
}

void http::write() {
    send(sockfd, &buffer[0], buffer.size(), 0);
    cout << "send message" <<endl;
}

void http::parse_request() {
    PARSE_STATE state = REQUEST_LINE;
    string str(buffer.begin(), buffer.end());
    regex ex_line("^([^ ]*) ([^ ]*) HTTP/([^ ]*)\r\n");
    regex ex_header("^([^:]*): ?(.*)\r\n");
    smatch result;
    string::const_iterator begin = str.begin();
    string::const_iterator end = str.end();
    while(state != FINISH) {
        switch(state) {
        case REQUEST_LINE:
            regex_search(begin, end, result, ex_line);
            mp["method"] = result[1];
            mp["url"] = result[2];
            mp["version"] = result[3];
            begin = result[0].second;
            state = HEADERS;
            break;    
        case HEADERS:
            regex_search(begin, end, result, ex_header);
            mp[result[1]] = result[2];
            begin = result[0].second;
            state = FINISH;
        }
    }
}

void respond_request() {
    
}