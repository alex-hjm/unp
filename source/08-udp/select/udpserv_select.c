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

#define	max(a,b)	((a) > (b) ? (a) : (b))

typedef void Sigfunc(int);

/* signal 信号函数*/
Sigfunc *signal(int signo, Sigfunc *func) {
  struct sigaction act, oact;
  act.sa_handler = func;  //设置处理函数
  /*设置为空集，意味着在该信号处理函数运行期间，不阻塞额外的信号。*/
  sigemptyset(&act.sa_mask);  //设置处理函数的信号掩码
  act.sa_flags = 0;
  if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
    //在被捕获的信号是SIGALRM时设置它。
    act.sa_flags |= SA_INTERRUPT; /* SunOS 4.x */
#endif
  } else {
#ifdef SA_RESTART
    //设置SA_RESTART标志
    //由相应信号中断的系统调用将由内核自动重启。
    act.sa_flags |= SA_RESTART; /* SVR4, 44BSD */
#endif
  }
  //调用sigaction函数
  if (sigaction(signo, &act, &oact) < 0) return (SIG_ERR);
  return (oact.sa_handler);
}

// 3. SIGCHLD的信号处理函数必须正确编写，应使用waitpid函数以免留下僵死进程。
void sig_chld(int signo) {
  pid_t pid;
  int stat;
  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    printf("child %d terminated\n", pid);
  return;
}

void str_echo(int sockfd) {
  ssize_t n;
  char buf[MAXLINE];
again:  //读入缓冲区并回射其中内容
  while ((n = read(sockfd, buf, MAXLINE)) > 0) write(sockfd, buf, n);
  if (n < 0 && errno == EINTR)
    goto again;
  else if (n < 0) {
    printf("str_echo: read error \n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  int listenfd, connfd, udpfd, nready, maxfdp1;
  char mesg[MAXLINE];
  pid_t childpid;
  fd_set rset;
  ssize_t n;
  socklen_t len;
  const int on = 1;
  struct sockaddr_in cliaddr, servaddr;

  if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }  // if

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  /*设置SO_REUSEADDR以防该端口上己有连接存在*/
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    printf("setsockopt error \n");
    exit(1);
  }

  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    printf("bind error \n");
    exit(1);
  }  // if

  if (listen(listenfd, LISTENQ) < 0) {
    printf("listen error \n");
    exit(1);
  }

  if (signal(SIGCHLD, sig_chld) == SIG_ERR) { /* must call waitpid() */
    printf("signal error \n");
    exit(1);
  }

  FD_ZERO(&rset);
  maxfdp1 = max(listenfd, udpfd) + 1;

  for (;;) {
    FD_SET(listenfd, &rset);
    FD_SET(udpfd, &rset);
    if (nready = select(maxfdp1, &rset, NULL, NULL, NULL) < 0) {
      if (errno == EINTR)
        continue; /* back to for() */
      else {
        printf("select error \n");
        exit(1);
      }
    }

    if (FD_ISSET(listenfd, &rset)) { /* new client connection */
      len = sizeof(cliaddr);
      if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len)) < 0) {
        printf("accept error \n");
        exit(1);
      }

      printf("new client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
             ntohs(cliaddr.sin_port));

      if ((childpid = fork()) == 0) { /* child process */
        if (close(listenfd) == -1) {  /* close listening socket */
          printf("close error \n");
          exit(1);
        }
        str_echo(connfd); /* process the request*/
        exit(0);
      }                          // if
      if (close(connfd) == -1) { /* parent closes connected socket */
        printf("close error \n");
        exit(1);
      }
    }

    if (FD_ISSET(udpfd, &rset)) {  //处理数据报的到达
      len = sizeof(cliaddr);
      if ((n = recvfrom(udpfd, mesg, MAXLINE, 0, (struct sockaddr *)&cliaddr,
                        &len)) < 0) {
        printf("recvfrom error \n");
        exit(1);
      }

      if (sendto(udpfd, mesg, n, 0, (struct sockaddr *)&cliaddr, len) != n) {
        printf("sendto error \n");
        exit(1);
      }
    }
  }
}
