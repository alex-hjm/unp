#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE 4096
#define SERV_PORT 9877
#define LISTENQ 1024  //最大客户连接数

void str_echo(int sockfd) {
  ssize_t n;
  char buf[MAXLINE];
again:  //读入缓冲区并回射其中内容
  while ((n = read(sockfd, buf, MAXLINE)) > 0) 
		write(sockfd, buf, n);
  if (n < 0 && errno == EINTR)
    goto again;
  else if (n < 0) {
    printf("str_echo: read error \n");
    exit(1);
  }
}

static void * doit(void *arg)
{
	int		connfd;
	connfd = *((int *) arg);
	free(arg);
	pthread_detach(pthread_self());
	str_echo(connfd);		/* same function as before */
	close(connfd);			/* done with connected socket */
	return(NULL);
}

int main(int argc, char **argv) {
  /*声明服务器地址和客户链接地址*/
  struct sockaddr_in servaddr, cliaddr;

  /*声明服务器监听套接字和客户端链接套接字*/
  int listenfd, *connfd;

  pthread_t		tid;

  /*声明缓冲区*/
  char buf[MAXLINE];

  socklen_t clilen;

  /*(1) 初始化监听套接字listenfd*/
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }  // if

  /*(2) 设置服务器sockaddr_in结构*/
  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //表明可接受任意IP地址
  servaddr.sin_port = htons(SERV_PORT);

  /*(3) 绑定套接字和端口*/
  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("bind error \n");
    exit(1);
  }  // if

  /*(4) 监听客户请求*/
  if (listen(listenfd, LISTENQ) < 0) {
    printf("listen error \n");
    exit(1);
  }  // if

  /*(5) 接受客户请求*/
  for (;;) {
    clilen = sizeof(cliaddr);
    connfd = (int*)malloc(sizeof(int));
    if ((*connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
      if (errno == EINTR)  // 2. 当捕获信号时，必须处理被中断的系统调用:
        continue;
      else {
        printf("accept error \n");
        exit(1);
      }
			pthread_create(&tid, NULL, &doit, connfd);
    }  // if
  }  // for
}
