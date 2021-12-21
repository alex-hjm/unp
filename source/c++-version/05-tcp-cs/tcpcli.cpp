#include <iostream>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 4096
#define LISTENQ 1024  //最大客户连接数
#define SERV_PORT 9877

/*readline函数实现*/
ssize_t readline(int fd, char *vptr, size_t maxlen) 
{
  ssize_t n, rc;
  char c, *ptr;

  ptr = vptr;
  for (n = 1; n < maxlen; n++) {
    if ((rc = read(fd, &c, 1)) == 1) {
      *ptr++ = c;
      if (c == '\n') break; /* newline is stored, like fgets() */
    } else if (rc == 0) {
      *ptr = 0;
      return (n - 1); /* EOF, n - 1 bytes were read */
    } else
      return (-1); /* error, errno set by read() */
  }

  *ptr = 0; /* null terminate like fgets() */
  return (n);
}

int main(int argc, char **argv) 
{
  /*声明套接字和链接服务器地址*/
  int sockfd;
  struct sockaddr_in servaddr;

  /*判断是否为合法输入*/
  if (argc != 2) {
    printf("usage: tcpcli <IPaddress> \n");
    exit(1);
  }  // if

  /*(1) 创建套接字*/
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }  // if

  /*(2) 设置链接服务器地址结构*/
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);
  if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0) {
    printf("inet_pton error for %s\n", argv[1]);
    exit(1);
  }  // if

  /*(3) 发送链接服务器请求*/
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("connect error");
    exit(1);
  }  // if

  /*(4) 消息处理 str_cli*/
  char sendline[MAXLINE], recvline[MAXLINE];

  while (fgets(sendline, MAXLINE, stdin) != NULL) {
    write(sockfd, sendline, strlen(sendline));
    //从服务器读入回射行，写到标准输出
    if (readline(sockfd, recvline, MAXLINE) == 0) {
      printf("server terminated prematurely");
      exit(1);
    }  // if

    if (fputs(recvline, stdout) == EOF) {
      printf("fputs error");
      exit(1);
    }  // if
  }    // while

  /*(5) 关闭套接字*/
  if (close(sockfd) == -1) {
    printf("close error");
    exit(1);
  }
}
