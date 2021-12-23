#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 4096
#define LISTENQ 1024  //最大客户连接数
#define SERV_PORT 9877
#define INFTIM -1
#define EPOLL_SIZE 50

void set_nonblock(int sockfd) { /*设置非阻塞边缘触发*/
  int flag;
  flag = fcntl(sockfd, F_GETFL, 0);
  if (flag < 0) {
    printf("fcntl get error \n");
    return;
  }  // if

  if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
    printf("fcntl set error \n");
    return;
  }  // if
}

int main(int argc, char **argv) {
  int listenfd, connfd, sockfd, epfd;
  int i, maxi, nfds;
  ssize_t n;
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in servaddr, cliaddr;

  /*声明epoll_event结构体变量，ev用于注册事件，数组用于回传要处理的事件*/
  struct epoll_event ev, events[EPOLL_SIZE];

  /*(1) 初始化监听套接字listenfd*/
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }  // if

  set_nonblock(listenfd);

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

  /*(4) 设置epoll*/
	epfd = epoll_create(EPOLL_SIZE);/*生成用于处理accept的epoll专用文件描述符*/	
	ev.data.fd = listenfd;/*设置监听描述符*/
	ev.events = EPOLLIN | EPOLLET;/*设置处理事件类型 ET模式*/
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev)<0) {	/*注册事件*/	
    printf("epoll_ctl error \n");
    exit(1);
  }

  /*(5) 接受客户请求*/
  for (;;) {
    if (nfds = epoll_wait(epfd , events , EPOLL_SIZE , -1) < 0) {
      printf("epoll_wait error \n");
      exit(1);
    }

    for(i=0 ; i<nfds ; ++i) {
      if(events[i].data.fd == listenfd) {	/* new client connection */
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
          printf("accept error \n");
          exit(1);
        }
        printf("new client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
              ntohs(cliaddr.sin_port));

        set_nonblock(connfd);/*设置为非阻塞*/
				ev.data.fd = connfd;
				ev.events = EPOLLIN | EPOLLET;
				if(epoll_ctl(epfd , EPOLL_CTL_ADD , connfd , &ev)<0) {
          printf("epoll_ctl error \n");
          exit(1);
        }

      } else if(events[i].events & EPOLLIN) { /* new data in */
        if((sockfd = events[i].data.fd) < 0)
					continue;
        if ((n = read(sockfd, buf, MAXLINE)) < 0) {
          if (errno == ECONNRESET) {/*4connection reset by client */
            printf("client[%d] aborted connection\n", i);
            if (close(sockfd) == -1) {
              printf("close error \n");
              exit(1);
            }
            events[i].data.fd = -1;
          } else {
            printf("read error \n");
            exit(1);
          }
        } else if (n == 0) { /*4connection closed by client */
          printf("client[%d] closed connection\n", i);
          if (close(sockfd) == -1) {
            printf("close error \n");
            exit(1);
          }
          events[i].data.fd = -1;
        } else {
          printf("clint[%d] send message: %s\n", i, buf);
          ev.data.fd = sockfd;/*设置用于注册写操作文件描述符和事件*/
					ev.events = EPOLLOUT| EPOLLET;	
					if(epoll_ctl(epfd , EPOLL_CTL_MOD , sockfd , &ev)<0) {
            printf("epoll_ctl error \n");
            exit(1);
          }
        }
      } else if (events[i].events & EPOLLOUT) { /* new data out */
        if((sockfd = events[i].data.fd) < 0)
				  continue;
        if (write(sockfd, buf, n) != n) {
            printf("write error \n");
            break;
          }  // if
        ev.data.fd = sockfd;/*设置用于注册读操作文件描述符和事件*/
				ev.events = EPOLLIN | EPOLLET;
				if(epoll_ctl(epfd , EPOLL_CTL_MOD , sockfd , &ev)<0) {
          printf("epoll_ctl error \n");
          exit(1);
        }		
      }
    }
  }
}
