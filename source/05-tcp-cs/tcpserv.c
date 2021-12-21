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
  /*声明服务器地址和客户链接地址*/
  struct sockaddr_in servaddr, cliaddr;

  /*声明服务器监听套接字和客户端链接套接字*/
  int listenfd, connfd;

  pid_t childpid;

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

  // 1. 当fork子进程时，必须捕获SIGCHLD信号
  if (signal(SIGCHLD, sig_chld) == SIG_ERR) {
    printf("signal error \n");
    exit(1);
  }

  /*(5) 接受客户请求*/
  for (;;) {
    clilen = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
      if (errno == EINTR)  // 2. 当捕获信号时，必须处理被中断的系统调用:
        continue;
      else {
        printf("accept error \n");
        exit(1);
      }
    }  // if

    //新建子进程单独处理链接
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
  }  // for

  /*(6) 关闭监听套接字*/
  if (close(listenfd) == -1) {
    printf("close error \n");
    exit(1);
  }
}
