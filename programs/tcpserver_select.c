#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* server side parameters */
#define SERVER_PORT	12345
#define LISTENQ     2         /* 2nd argument to listen () */
#define BUFFSIZE    8192         /* buffer size for reads and writes */

#define	MAX_CLIENTS	1000
#define	SELECT_TIMEOUT 10
#define SERVERPORT 9999
int str_echo(int fd);

int main(int argc, char **argv) {
	int     listenfd, connfd;
	struct sockaddr_in cliaddr, servaddr;
	socklen_t clilen;

	fd_set	rset, wset; /* master read and write fdset */
	fd_set	wk_rset, wk_wset; /* working read and write fdset */
	struct timeval timeout = {10, 0};

	int max_fd; /* max socket fd value */
	int client_sock[MAX_CLIENTS];
	int cnt;  /* count of sockets of interest at an invocation */
	int status;

	/* initialize the client socket list */
	for  (int ii = 0; ii < MAX_CLIENTS; ii++) {
		client_sock[ii] = 0;
	}
	FD_ZERO(&rset); FD_ZERO(&wset);

	bzero((void *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip_addr);
	servaddr.sin_port = htons(port);

	listenfd = socket (AF_INET, SOCK_STREAM, 0);
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
		timeout.tv_sec = SELECT_TIMEOUT;
		timeout.tv_usec = 0;

		cnt = select(max_fd, &wk_rset, &wk_wset, NULL, &timeout);
    if (cnt == 0) {
			printf("Select timeout occurred for timeout of %ds\n", SELECT_TIMEOUT);
			continue;
		}
		/* some sockets are of interest */
		/* check if new connection has arrived */
		if (FD_ISSET(listenfd, &wk_rset)) {
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
			/* new connection is to be made part of master set */
      ntohs(cliaddr.sin_port));
			client_sock[connfd] = connfd; /* any true value */
			max_fd = connfd + 1;

			FD_SET(connfd, &rset);
			FD_SET(connfd, &wset);
		}

		/** the main processing of server sockets
		 * for each socket which is returned by select, read the data and respond back
		 */
		 for (int ifd = 0; ifd < max_fd; ifd++ ) {
		 	if (ifd == listenfd) {
				continue;
			}
			if (FD_ISSET(ifd, &wk_rset)) {
				status = str_echo(ifd);
				if ((status <= 0) && (errno != EINTR)) {
					FD_CLR(ifd, &rset);
					FD_CLR(ifd, &wset);
					close(ifd);
				}
			}
		}

	}
}

/* just do one read. For subsequent read, this function will be invoked again */
int str_echo(int clientfd) {
	char buf[BUFFSIZE];
	int size;

	bzero(buf, sizeof(buf));
	size = recv(clientfd, buf, sizeof(buf), 0);
	send(clientfd, buf, strnlen(buf, sizeof(buf)), 0);
}
