#include "http.h"

using namespace std;

string http::srcDir;
atomic<int> http::userCount(0);

void http::init(int connfd, struct sockaddr_in connaddr, int epollfd) {
    sockfd = connfd;
    addr = connaddr;
    this->epollfd = epollfd;
    event.data.fd = sockfd;
    event.events = EPOLLOUT | EPOLLET | EPOLLONESHOT; 
    file_buffer = nullptr;
    srcDir = getcwd(nullptr, 256);
    srcDir += "/resources";
    userCount ++;
    // cout << srcDir << endl;
    // cout << "get client" << endl;
}

void http::Close() {
    buffer.clear();
    write_buffer.clear();
    userCount --;   
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
    close(sockfd);
    munmap(file_buffer, fileStat.st_size);
    file_buffer = nullptr;
    cout << userCount << endl;
    mp.clear();
    // cout << "discard client" << endl;
    cout << "***************************************************" << endl;
}

void http::task() {
    char temp[1024] = {0};
    int len;
    while((len = recv(sockfd, temp, 1023, 0)) > 0) {
        temp[len] = '\0';
        for(int i = 0; i < len; ++ i) buffer.push_back(temp[i]);
    }
    // cout << "accept message" << endl;
    // cout << string(buffer.begin(), buffer.end()) << endl;

    parse_request();
    respond_request();
    epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
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
            cout << result[1] << " " << result[2] << endl; //
            mp["method"] = result[1];
            mp["url"] = result[2];
            if (mp["url"] == "/") mp["url"] = "/index.html";
            mp["version"] = result[3];
            begin = result[0].second;
            state = HEADERS;
            break;    
        case HEADERS:
            regex_search(begin, end, result, ex_header);
            mp[result[1]] = result[2];
            begin = result[0].second;
            if (begin == end) state = BODY;
            break;
        case BODY:
            parse_body(str.substr(begin-str.begin()));
            state = FINISH;
            break;
        case FINISH:
            break;
        }
    }
}

void http::parse_body(string body) {
    if(body.size() == 0) { return; }
    string key, value;
    int num = 0;
    int n = body.size();
    int i = 0, j = 0;   
    for(; i < n; i++) {
        char ch = body[i];
        switch (ch) {
        case '=':
            key = body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body[i] = ' ';
            break;
        case '%':
            num = ConverHex(body[i + 1]) * 16 + ConverHex(body[i + 2]);
            body[i + 2] = num % 10 + '0';
            body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body.substr(j, i - j);
            j = i + 1;
            mp[key] = value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(mp.count(key) == 0 && j < i) {
        value = body.substr(j, i - j);
        mp[key] = value;
    }
}

int http::ConverHex(char ch) {
    if (ch <= 'f' && ch >= 'a') {
        return ch - 'a' + 10;
    }
    if (ch <= 'F' && ch >= 'A') {
        return ch - 'A' + 10;
    }
    return ch - '0';
}

string http::get_file_type(string file) {
    string::size_type idx = file.find_last_of('.');
    if(idx == string::npos) {
        return "text/plain";
    }
    return file.substr(idx);
}

void http::respond_request() {
    // 添加状态行
    pair<string,string> status("200", "OK");
    string path = mp["url"];
    if (get_file_type(path) == "text/plain") {
        path += ".html";
    }
    if(stat((srcDir + path).data(), &fileStat) < 0) { // 文件是否存在
        status = {"404", "Not Found"};
        path = "/404.html";
    }
    else if (S_ISDIR(fileStat.st_mode)) { // 文件是一个目录
        status = {"404", "Not Found"};
        path = "/404.html";
    }
    else if(!(fileStat.st_mode & S_IROTH)) { // 是否拥有权限
        status = {"403", "Forbidden"};
        path = "/403.html";
    }
    stat((srcDir + path).data(), &fileStat);
    string str = "HTTP/1.1 " + status.first + " " + status.second + "\r\n";
    // 添加头部和空行
    str += "Connection: keep-alive\r\n";
    str += "keep-alive: max=6, timeout=120\r\n";
    str += "Content-type: " + get_file_type(path) + "\r\n";
    str += "Content-length: " + to_string(fileStat.st_size) + "\r\n\r\n";
    for(auto i: str) {write_buffer.push_back(i);}
    // 添加消息主体
    // cout << "path " << (srcDir + path).data() << endl;
    int srcfd = open((srcDir + path).data(), O_RDONLY);
    assert(srcfd != -1);
    // 将文件映射到内存提高文件的访问速度 
    // MAP_PRIVATE 建立一个写入时拷贝的私有映射
    int* mmRet = (int*)mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    assert(*mmRet != -1);
    file_buffer = (char*) mmRet;

    // 集中写
    iv[0].iov_base = &write_buffer[0];
    iv[0].iov_len = write_buffer.size();
    iv[1].iov_base = file_buffer;
    iv[1].iov_len = fileStat.st_size;

    close(srcfd);
}


void http::write_() {
    cout << "send message" <<endl;
    while(true) {
        int len = writev(sockfd, iv, 2);
        // cout << len << " " << iv[0].iov_len << " " << iv[1].iov_len << endl;
        if (len == iv[0].iov_len + iv[1].iov_len) {
            break;
        }
        if (len == -1) {
            if (errno == EAGAIN) {
                epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
                return ;
            }
            break;
        }
        for(int i = 0; i < 2; ++ i) {
            if (len == 0) break;
            if (iv[i].iov_len == 0) continue;
            if (static_cast<size_t>(len) >= iv[i].iov_len) {
                len -= iv[i].iov_len;
                iv[i].iov_len = 0;
            }
            else {
                iv[i].iov_base = (uint8_t*) iv[i].iov_base + len;
                iv[i].iov_len -= len;
                len = 0;
            }
        }
    }
    Close();
}

