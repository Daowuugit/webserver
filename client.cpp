#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <arpa/inet.h>  
#include <sys/socket.h>  
  
int main() {  
    // 创建套接字    
	int sock = socket(AF_INET, SOCK_STREAM, 0);  

	// 与服务器建立连接    
	struct sockaddr_in serv_addr;  
	memset(&serv_addr, 0, sizeof serv_addr);  
	serv_addr.sin_family = AF_INET;  
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  
	serv_addr.sin_port = htons(1234);  
	connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));  
	  
	// 向服务器发送请求    
	char buffer[40] = {0};  
	printf("please input:\n");  
	scanf("%s", buffer);  
	send(sock, buffer, strlen(buffer), 0);  
	shutdown(sock, SHUT_WR);

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