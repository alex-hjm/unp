#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 4096
#define LISTENQ 1024  //最大客户连接数
#define SERV_PORT 9877

int main(int argc, char **argv) {
  int i, maxi, maxfd, listenfd, connfd, sockfd;
  int nready, client[FD_SETSIZE];
  ssize_t n;
  fd_set rset, allset;
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in servaddr, cliaddr;

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

  /*(4) 设置select*/
  maxfd = listenfd; /* initialize */
  maxi = -1;        /* index into client[] array */
  for (i = 0; i < FD_SETSIZE; i++)
    client[i] = -1; /* -1 indicates available entry */
  FD_ZERO(&allset);
  FD_SET(listenfd, &allset);

  /*(5) 接受客户请求*/
  for (;;) {
    rset = allset;
    if (nready = select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
      printf("select error \n");
      exit(1);
    }

    if (FD_ISSET(listenfd, &rset)) { /* new client connection */
      clilen = sizeof(cliaddr);
      if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) <
          0) {
        printf("accept error \n");
        exit(1);
      }

      printf("new client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
             ntohs(cliaddr.sin_port));

      for (i = 0; i < FD_SETSIZE; i++)
        if (client[i] < 0) {  //用第一个未用项记录该连接描述符
          client[i] = connfd; /* save descriptor */
          break;
        }
      if (i == FD_SETSIZE) {
        printf("too many clients \n");
        exit(1);
      }

      FD_SET(connfd, &allset);            /* add new descriptor to set */
      if (connfd > maxfd) maxfd = connfd; /* for select */
      if (i > maxi) maxi = i;             /* max index in client[] array */
      if (--nready <= 0)  //使用select返回值来避免检查未就绪的描述符
        continue;         /* no more readable descriptors */
    }                     // for

    for (i = 0; i <= maxi; i++) { /* check all clients for data */
      if ((sockfd = client[i]) < 0) continue;
      if (FD_ISSET(sockfd, &rset)) {  //若客户关闭连接，则更新数据结构
        n = read(sockfd, buf, MAXLINE);
        if (n == -1) {
          printf("read error \n");
          exit(1);
        } else if (n == 0) { /*4connection closed by client */
          if (close(sockfd) == -1) {
            printf("close error \n");
            exit(1);
          }
          FD_CLR(sockfd, &allset);
          client[i] = -1;
        } else {
          printf("clint[%d] send message: %s\n", i, buf);
          if (write(sockfd, buf, n) != n) {
            printf("write error \n");
            break;
          }  // if
        }
        if (--nready <= 0) break; /* no more readable descriptors */
      }
    }
  }
}
