#include "http.h"

using namespace std;

string http::srcDir;
atomic<int> http::userCount(0);

const unordered_map<string, string> http::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "}
};

void http::init(int connfd, struct sockaddr_in connaddr, int epollfd) {
    isClosed = false;
    sockfd = connfd;
    addr = connaddr;
    this->epollfd = epollfd;
    event_in.data.fd = sockfd;
    event_in.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP; 
    event_out.data.fd = sockfd;
    event_out.events = EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP; 
    file_buffer = nullptr;
    srcDir = getcwd(nullptr, 256);
    srcDir += "/../resources";
    userCount ++;
    isKeepAlive = false;
    // cout << srcDir << endl;
    // cout << "get client" << endl;
}

void http::Close() {
    if (!isClosed) {
        isClosed = true;
        write_close();
        userCount --;   
        cout << userCount << endl;
        close(sockfd);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
        cout << "***************************************************" << endl;
    }    
    // cout << "discard client" << endl;
}

void http::task_init() {
    // cout << userCount << endl;
    buffer.clear();
    mp.clear();
}

void http::task() {
    task_init();
    char temp[1024] = {0};
    int len;
    while((len = recv(sockfd, temp, 1023, 0)) > 0) {
        temp[len] = '\0';
        for(int i = 0; i < len; ++ i) buffer.push_back(temp[i]);
    }
    if (errno != EAGAIN) {
        Close();
        return ;
    }
    cout << "accept message" << endl;
    // cout << string(buffer.begin(), buffer.end()) << endl;

    if (parse_request()) {
        if (mp.count("Connection") && mp["Connection"] == "keep-alive" && mp["version"] == "1.1") {
            isKeepAlive = true;
        }
        respond_request({"200", "OK"});
    }
    else {
        isKeepAlive = false;
        respond_request({"400", "Bad Request"});
    }
    epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event_out);
}


bool http::parse_request() {
    PARSE_STATE state = REQUEST_LINE;
    string str(buffer.begin(), buffer.end());
    regex ex_line("^([^ ]*) ([^ ]*) HTTP/([^ ]*)\r\n");
    regex ex_header("^([^:]*): ?(.*)\r\n");
    regex ex_("^\r\n");
    smatch result;
    string::const_iterator begin = str.begin();
    string::const_iterator end = str.end();
    while(state != FINISH) {
        switch(state) {
        case REQUEST_LINE:
            if (regex_search(begin, end, result, ex_line)) {
                // cout << result[1] << " " << result[2] << endl; 
                mp["method"] = result[1];
                mp["url"] = result[2];
                if (mp["url"] == "/") mp["url"] = "/index.html";
                mp["version"] = result[3];
                begin = result[0].second;
                state = HEADERS;
            }
            else {
                return false;
            }
            break;    
        case HEADERS:
            if (regex_search(begin, end, result, ex_header)) {
                // cout << result[1] << " " << result[2] << endl;
                // for(int i = 0; i < result.size(); ++ i) {
                //     cout << result[i] << endl;
                // }
                mp[result[1]] = result[2];
                begin = result[0].second;
            }
            else if (regex_search(begin, end, result, ex_)){
                begin = result[0].second;
                state = BODY;
            }
            else {
                return false;
            }
            break;
        case BODY:
            //parse_body(str.substr(begin-str.begin()));
            state = FINISH;
            break;
        case FINISH:
            break;
        }
    }
    return true;
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
    string suf = file.substr(idx);
    if (SUFFIX_TYPE.count(suf)) {
        return SUFFIX_TYPE.find(suf)->second;
    }
    return suf;
}

void http::respond_request(pair<string,string> status) {
    // 添加状态行
    string path;
    if (status.first == "200") {
        path = mp["url"];
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
    }
    else if (status.first == "400") {
        path = "/400.html";
    }
    stat((srcDir + path).data(), &fileStat);
    string str = "HTTP/1.1 " + status.first + " " + status.second + "\r\n";
    // 添加头部和空行
    if (isKeepAlive) {
        str += "Connection: keep-alive\r\n";
    }
    else {
        str += "Connection: close\r\n";
    }
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

void http::write_close() {
    mp.clear();
    write_buffer.clear();
    munmap(file_buffer, fileStat.st_size);
    file_buffer = nullptr;
}

void http::write_() {
    cout << "send message" <<endl;
    cout << userCount << endl;
    while(true) {
        int len = writev(sockfd, iv, 2);
        // cout << len << " " << iv[0].iov_len << " " << iv[1].iov_len << endl;
        if (len == -1) {
            if (errno == EAGAIN) {
                epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event_out);
                return ;
            }
            Close();
            return ;
        }
        if (len == iv[0].iov_len + iv[1].iov_len) {
            if (isKeepAlive) {
                write_close();
                epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event_in);
            }
            else {
                Close();
            }
            return ;
        }
        else {
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
    }
}

