#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAXLINE 4096
#define LISTENQ 1024  //最大客户连接数

int main(int argc, char **argv) 
{
  int listenfd, connfd;
  struct sockaddr_in servaddr, cliaddr;
  char buff[MAXLINE];
  time_t ticks;
  socklen_t len;

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(13); /* daytime server */

  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("bind error \n");
    exit(1);
  }

  if (listen(listenfd, LISTENQ) < 0) {
    printf("listen error \n");
    exit(1);
  }

  while (1) {
    len = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len)) < 0) {
      printf("accept error \n");
      exit(1);
    }

    printf("connection from %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
           ntohs(cliaddr.sin_port));

    ticks = time(NULL);
    //在字符串末尾添加一个回车符和一个回行符
    snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

    if (write(connfd, buff, strlen(buff)) != strlen(buff)) {
      printf("write error \n");
      exit(1);
    }

    if (close(connfd) == -1) {
      printf("close error \n");
      exit(1);
    }
  }
}
