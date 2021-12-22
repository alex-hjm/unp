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

void dg_cli(FILE *fp, int sockfd, const struct sockaddr *pservaddr,
            socklen_t servlen) 
{
  int n;
  char sendline[MAXLINE], recvline[MAXLINE + 1];

  while (fgets(sendline, MAXLINE, fp) != NULL) {
    if (sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen) !=
        strlen(sendline)) {
      printf("sendto error");
      exit(1);
    }  // if

    if ((n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) < 0) {
      printf("recvfrom error");
      exit(1);
    }  // if

    recvline[n] = 0; /* null terminate */
    fputs(recvline, stdout);
  }  // while
}

int main(int argc, char **argv) 
{
  int sockfd, t;
  struct sockaddr_in servaddr;
  if (argc != 2) {
    printf("usage: udpcli <IPaddress> \n");
    exit(1);
  }  // if

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);

  if ((t = inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0) {
    printf("inet_pton error \n");
    exit(1);
  }  // if

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf("socket error \n");
    exit(1);
  }  // if

  dg_cli(stdin, sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  exit(0);
}
