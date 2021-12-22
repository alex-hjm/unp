#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 4096
#define SERV_PORT 9877

void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen) 
{
  int n;
  socklen_t len;
  char mesg[MAXLINE];
  for (;;) {
    len = clilen;
    if ((n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len)) < 0) {
      printf("recvfrom error \n");
      exit(1);
    }

    if (sendto(sockfd, mesg, n, 0, pcliaddr, len) != (ssize_t)n) {
      printf("sendto error \n");
      exit(1);
    }
  }
}

int main(int argc, char **argv) 
{
  int sockfd;
  struct sockaddr_in servaddr, cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }  // if

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    printf("bind error \n");
    exit(1);
  }  // if

  dg_echo(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
}
