#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <arpa/inet.h>  
#include <sys/socket.h>  
#include <assert.h>

int main() {  
    const char* ip = "127.0.0.1";
    int port = 5555;

	int sock = socket(PF_INET, SOCK_STREAM, 0);  
    assert(sock >= 0);
	struct sockaddr_in address;  
    bzero(&address, sizeof address); 
    address.sin_family = AF_INET;  
    inet_pton(AF_INET, ip, &address.sin_addr); 
	address.sin_port = htons(port);  
	connect(sock, (struct sockaddr*)&address, sizeof(address));  
	  
	// 向服务器发送请求    
	char buffer[40] = {0};  
	printf("please input:\n");  
	scanf("%s", buffer);  
	send(sock, buffer, strlen(buffer), 0);  

	// 读取服务器传回的数据
	printf("Message from server: \n");  
	char acc[40] = {0};
	int len; 
	while((len = recv(sock, acc, sizeof(acc)-1, 0)) > 0) {
		acc[len] = '\0';
		printf("%s\n", acc);
	}
	if (len == -1) {
		printf("recv error\n");
		exit(1);
	}
	close(sock);
}