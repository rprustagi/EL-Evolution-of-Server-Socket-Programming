/**
 * server uses poll() to deal with multiple clients.
 * it doesn not create a new process/thread for each client.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

/* server parameters */
#define SERVER_PORT	12345
#define LISTENQ     2         /* 2nd argument to listen () */
#define BUFFSIZE    8192         /* buffer size for reads and writes */
#define	MAX_CLIENTS	1000
#define	POLL_TIMEOUT 10000	/* in milli seconds */

int str_echo(int fd);

int main(int argc, char **argv) {
	int     listenfd, connfd;
	pid_t   childpid;
	struct sockaddr_in cliaddr, servaddr;
	socklen_t clilen;
	int	child_status;
	int opt;
	char *ip_addr = (char *) NULL;
	short port = 0;
	struct timeval timeout;

	int	ii; /* index variable */
	int cnt;  /* count of sockets of interest at an invocation */
	int status;
	struct pollfd client[MAX_CLIENTS];
	int max_ifd;

	while ((opt = getopt(argc, argv, "i:p:")) != -1) {
		switch (opt) {
		case 'i':
			ip_addr = optarg;
			break;

		case 'p':
			port = atoi(optarg);
			break;

		default:
			fprintf(stderr, "Usage: %s -i ipaddr -p port\n", argv[0]);
			exit(1);
			break;
		}
	}

	if ((port == 0) || (ip_addr == (char *)NULL)) {
		fprintf(stderr, "ipaddr/port num not specified or invalid value\n");
		exit(1);
	}

	/* initialize the client socket list */
	for (ii=0; ii<MAX_CLIENTS; ii++) {
		client[ii].fd = -1; /* invalid FD */
	}

	listenfd = socket (AF_INET, SOCK_STREAM, 0);

	bzero((void *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip_addr);
	servaddr.sin_port = htons(port);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);

	/* need to make listenfd as part of rset as it provides new connection */
	client[0].fd = listenfd;
	client[0].events = POLLIN;
	max_ifd = 0; /* max index into the client array */
	
	clilen = sizeof(cliaddr);
	for (; ; ) {
		 cnt = poll(client, max_ifd + 1, POLL_TIMEOUT);

		if (cnt == -1) {
			fprintf(stderr, "Error %d in select()", errno);
			exit(1);
		} else if (cnt == 0) {
			printf("poll timeout occurred for timeout of %dms\n", POLL_TIMEOUT);
			continue;
		}
		/* some sockets are of interest */
		/* check if new connection has arrived */
		if (client[0].revents & POLLIN) {
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
			/* new connection is to be made part of master set */
			printf("new connection from client %s:%d accepted\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

			for (ii=0; ii < MAX_CLIENTS; ii++) {
				if (client[ii].fd < 0) {
					client[ii].fd = connfd;
					client[ii].events = POLLIN;
					break;
				}
			}
			if (ii >= MAX_CLIENTS) {
				fprintf(stderr, "Connection from client %s exceed capacity\n", inet_ntoa(cliaddr.sin_addr));
				close(connfd);
				continue;
			}
			if (ii > max_ifd) {
				max_ifd = ii;
			}
			if (--cnt <= 0) {
				/* processed all required inputs */
				continue;
			}
		}

		/** the main processing of server sockets 
		 * for each socket which is returned by select, read the data and respond back
		 */
		 for (ii = 1; ii <= max_ifd; ii++ ) {
			if (client[ii].fd < 0) {
				continue;
			}
			/* read the data from the client if available */
			if (client[ii].revents & POLLIN) {
				status = str_echo(client[ii].fd);
				if (status == 0) { /* client has closed the connection */
					close(client[ii].fd);
					client[ii].fd = -1;
				}
				if (--cnt <= 0) {
					/* processed all required inputs */
					break;;
				}
			}
			if (client[ii].revents & POLLERR) {
				close(client[ii].fd);
				client[ii].fd = -1;
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
	if (size > 0) {
		send(clientfd, buf, strnlen(buf, sizeof(buf)), 0);
	}
}
