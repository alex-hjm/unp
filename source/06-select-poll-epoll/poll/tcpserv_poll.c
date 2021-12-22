#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
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
#define INFTIM -1
#define OPEN_MAX 1024

int main(int argc, char **argv) {
  int i, maxi, listenfd, connfd, sockfd;
  int nready;
  ssize_t n;
  char buf[MAXLINE];
  socklen_t clilen;
  struct pollfd client[OPEN_MAX];
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

  /*(4) 设置poll*/
  client[0].fd = listenfd;        //第一项用于监听套接字
  client[0].events = POLLRDNORM;  //获取连接通知
  for (i = 1; i < OPEN_MAX; i++)
    client[i].fd = -1; /* -1 indicates available entry */
  maxi = 0;            /* max index into client[] array */

  /*(5) 接受客户请求*/
  for (;;) {
    if (nready = poll(client, maxi + 1, INFTIM) < 0) {
      printf("poll error \n");
      exit(1);
    }

    if (client[0].revents & POLLRDNORM) { /* new client connection */
      clilen = sizeof(cliaddr);
      if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
        printf("accept error \n");
        exit(1);
      }
      printf("new client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
             ntohs(cliaddr.sin_port));
      // client[0]用于监听套接字

      for (i = 1; i < OPEN_MAX; i++)
        if (client[i].fd < 0) {  //用第一个未用项记录该连接描述符
          client[i].fd = connfd; /* save descriptor */
          break;
        }
      if (i == OPEN_MAX) {
        printf("too many clients \n");
        exit(1);
      }

      client[i].events = POLLRDNORM;  /*该描述符等待的事件*/
      if (i > maxi) maxi = i;         /* max index in client[] array */

      if (--nready <= 0) continue;    /* no more readable descriptors */
    }

    for (i = 1; i <= maxi; i++) {     /* check all clients for data */
      if ((sockfd = client[i].fd) < 0) continue;
      if (client[i].revents & (POLLRDNORM | POLLERR)) {
        if ((n = read(sockfd, buf, MAXLINE)) < 0) {
          if (errno == ECONNRESET) {  /*4connection reset by client */
            printf("client[%d] aborted connection\n", i);
            if (close(sockfd) == -1) {
              printf("close error \n");
              exit(1);
            }
            client[i].fd = -1;
          } else {
            printf("read error \n");
            exit(1);
          }
        } else if (n == 0) {          /*4connection closed by client */
          printf("client[%d] closed connection\n", i);
          if (close(sockfd) == -1) {
            printf("close error \n");
            exit(1);
          }
          client[i].fd = -1;
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
