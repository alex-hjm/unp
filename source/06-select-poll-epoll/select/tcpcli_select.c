#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 4096
#define LISTENQ 1024  //最大客户连接数
#define SERV_PORT 9877
#define	max(a,b)	((a) > (b) ? (a) : (b))

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

/*采用select的客户端消息处理函数*/
void str_cli(FILE* fp , int sockfd)
{
  int   maxfdp1, stdineof;
	fd_set rset;
	char  buf[MAXLINE];
	int		n;

	stdineof = 0;//初始化为0的新标志
	FD_ZERO(&rset);

  for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
    if(select(maxfdp1, &rset, NULL, NULL, NULL)<0) {
      printf("select error");
			exit(1);
    }

    if (FD_ISSET(sockfd, &rset)) {	/* socket is readable */
      if ( (n = read(sockfd, buf, MAXLINE)) == 0) {//当在套接字上读到EOF时
        if (stdineof == 1)//如果已在标准输入上遇到EOF，则正常终止
					return;		/* normal termination */
				else {
          printf("str_cli: server terminated prematurely");
			    exit(1);
        }
			}
			if(write(fileno(stdout), buf, n)!=n) {
         printf("write error");
			exit(1);
      }
		}

    if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
      if ( (n = read(fileno(fp), buf, MAXLINE)) == 0) {//当在标准输入上读到EOF时
				stdineof = 1;//半（写）关闭并发送FIN
				if(shutdown(sockfd, SHUT_WR)<0) {	/* send FIN */
				 printf("shutdown error");
			exit(1);
        }
        FD_CLR(fileno(fp), &rset);
				continue;
			}

			if(write(sockfd, buf, n)!=n) {
         printf("write error");
			exit(1);
      }
		}
  }
}

int main(int argc, char **argv) {
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
  str_cli(stdin,sockfd);

  /*(5) 关闭套接字*/
  if (close(sockfd) == -1) {
    printf("close error");
    exit(1);
  }
}
