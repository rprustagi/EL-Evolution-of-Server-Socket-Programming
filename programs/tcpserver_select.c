/**
 * server uses select() to deal with multiple clients.
 * it doesn not create a new process/thread for each client.
 */

#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

/* server configuration parameters */
#define SERVER_PORT     9999
#define LISTENQ         2         /* 2nd argument to listen () */
#define BUFFSIZE        8192      /* buffer size for reads and writes */

#define  MAX_CLIENTS    1024
#define  SELECT_TIMEOUT 10

int str_echo(int fd);

int main(int argc, char **argv) {
  int     listenfd, connfd;
  pid_t   childpid;
  struct sockaddr_in cliaddr, servaddr;
  socklen_t clilen;
  int  child_status;
  int opt;
  char *ip_addr = (char *) NULL;
  short port = 0;
  fd_set  rset; /* master read fdset */
  fd_set  wset; /* master write fdset */
  fd_set  wk_rset; /* working read set */
  fd_set  wk_wset; /* working write set */
  struct timeval timeout;
  int select_timeout = SELECT_TIMEOUT;

  int max_fd; /* max socket fd value */
  int client_sock[MAX_CLIENTS];
  int ifd; /* fd of current interest */
  int  ii; /* index variable */
  int cnt;  /* count of sockets of interest at an invocation */
  int status;

  while ((opt = getopt(argc, argv, "i:p:t:")) != -1) {
    switch (opt) {
    case 'i':
      ip_addr = optarg;
      break;

    case 'p':
      port = atoi(optarg);
      break;

    case 't':
      select_timeout = atoi(optarg);
      break;

    default:
      fprintf(stderr, "Usage: %s -i <ipaddr> -p <port> -t <timeout(s)>\n", argv[0]);
      exit(1);
      break;
    } /* switch(opt) */
  } /* while (opt...) */

  if ((port == 0) || (ip_addr == (char *)NULL)) {
    fprintf(stderr, "error in  ipaddr or port number value\n");
    exit(1);
  }

  /* initialize the client socket list */
  for  (ii = 0; ii < MAX_CLIENTS; ii++) {
    client_sock[ii] = 0;
  }
  FD_ZERO(&rset);
  FD_ZERO(&wset);

  listenfd = socket (AF_INET, SOCK_STREAM, 0);

  bzero((void *)&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(ip_addr);
  servaddr.sin_port = htons(port);

  bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
  listen(listenfd, LISTENQ);

  /* need to make listenfd as part of rset as it provides new connection */
  FD_SET(listenfd, &rset);
  max_fd = listenfd + 1;
  
  clilen = sizeof(cliaddr);
  for (; ; ) {
    /* setup the working set */
    FD_ZERO(&wk_rset);
    FD_ZERO(&wk_wset);
    for (ii=0; ii < max_fd; ii++) {
      if (FD_ISSET(ii, &rset)) {
        FD_SET(ii, &wk_rset);
      }
    }
    /* set some timeout value */
    timeout.tv_sec = select_timeout;
    timeout.tv_usec = 0;

    cnt = select(max_fd, &wk_rset, &wk_wset, NULL, &timeout);
    if (cnt == -1) {
      fprintf(stderr, "Error %d in select()", errno);
      exit(1);
    } else if (cnt == 0) {
      printf("Select timeout occurred for timeout of %ds\n", select_timeout);
      continue;
    }
    /* some sockets are of interest */
    /* check if new connection has arrived */
    if (FD_ISSET(listenfd, &wk_rset)) {
      connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
      if (connfd >= sizeof(fd_set)*8) {
        fprintf(stderr, "connection from client %s, fd=%d exceeds fdset\n", inet_ntoa(cliaddr.sin_addr), connfd);
        close(connfd); /* beyond capacity of server */
        continue;
      }
      /* new connection is to be made part of master set */
      printf("connection from client %s:%d accepted, fd=%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), connfd);
      client_sock[connfd] = connfd; /* any true value */
      if (max_fd < connfd + 1) {
        max_fd = connfd + 1;
      }
      FD_SET(connfd, &rset);
      FD_SET(connfd, &wset);
    }

    /** the main processing of server sockets 
     * for each socket which is returned by select, read the data and respond back
     */
     for (ifd = 0; ifd < max_fd; ifd++ ) {
       if (ifd == listenfd) {
        continue;
      }
      if (FD_ISSET(ifd, &wk_rset)) {
        status = str_echo(ifd);
        if ((status <= 0) && (errno != EINTR)) {
          FD_CLR(ifd, &rset);
          FD_CLR(ifd, &wset);
          close(ifd);
          printf("Socket fd %d closed\n", ifd);
        } /* if status <=0 */
      } /* if FD_ISSET */
    } /* for ifd = 0; */
  } /* for (; ; ) */
} /* main */

/* just do one read. For subsequent read, this function will be invoked again */
int str_echo(int clientfd) {
  char buf[BUFFSIZE];
  int size;

  bzero(buf, sizeof(buf));
  size = recv(clientfd, buf, sizeof(buf), 0);
  send(clientfd, buf, strnlen(buf, sizeof(buf)), 0);
}
