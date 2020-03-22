/**
 * server uses epoll() to deal with multiple clients.
 *  this should be the most efficient way of dealing with large number of clients 
 */

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
#include <sys/epoll.h>

/* server parameters */
#define SERVER_PORT	12345
#define LISTENQ     2         /* 2nd argument to listen () */
#define BUFFSIZE    8192         /* buffer size for reads and writes */
#define	MAX_EVENTS	100
#define	EPOLL_TIMEOUT 60

int str_echo(int fd);

int main(int argc, char **argv) {
	int     listenfd, connfd;
	pid_t   childpid;
	struct sockaddr_in cliaddr, servaddr;
	socklen_t clilen;
	int opt;
	char *ip_addr = (char *) NULL;
	short port = 0;
  	int efd;	/* epoll instance fd */
  	struct epoll_event event; /* to register events for new fd */
  	struct epoll_event revents[MAX_EVENTS]; /* returned events on epoll_wait */

	int	ii; /* index variable */
	int cnt;  /* count of fd of interest */
	int status;

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
		fprintf(stderr, "ipaddr/port number not specified or invalid\n");
		exit(1);
	}

	listenfd = socket (AF_INET, SOCK_STREAM, 0);

	bzero((void *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip_addr);
	servaddr.sin_port = htons(port);

	status = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if (status != 0) {
		fprintf(stderr, "Error in binding to ip addr %s and port %d\n", ip_addr, port);
		exit(1);
	}
	status = listen(listenfd, LISTENQ);
	if (status != 0) {
		fprintf(stderr, "Error in listen with Q Size %d\n", LISTENQ);
		exit(1);
	}

	/* Create the epoll instance */
	if ((efd = epoll_create(MAX_EVENTS)) < 0) {
		fprintf(stderr, "Error in creating epoll instance\n");
		exit(1);
	}
	/* Define the events for incoming new requests from clients */
  	event.data.fd = listenfd;
  	event.events = EPOLLIN; /* by default, Level triggered */
  	status = epoll_ctl (efd, EPOLL_CTL_ADD, listenfd, &event);
  	if (status != 0) {
      fprintf(stderr, "error epoll_ctl: adding fd %d\n", listenfd);
	  exit(1);
	}
	
	clilen = sizeof(cliaddr);
	/* process new client requests as well as existing clients */
	for (; ; ) {
    	cnt = epoll_wait (efd, revents, MAX_EVENTS, -1);

		if (cnt == -1) {
			fprintf(stderr, "Error in epoll_wait()\n");
			exit(1);
		} else if (cnt == 0) {
			fprintf(stderr, "epoll_wait: error, timeout should not occur\n");
			continue;
		}
#if 0
		if (cnt > 1) {
			fprintf(stderr, "epoll_wait: Received %d events\n", cnt);
		}
#endif
		/* process all events of interest */
		for (ii = 0; ii < cnt; ii++) {
	  		if ((revents[ii].events & EPOLLERR) ||
              (revents[ii].events & EPOLLHUP) ||
              (!(revents[ii].events & EPOLLIN))) {
              	/* An error has occured on this fd, or the socket is not
                 ready for reading (why were we notified then?) */
	      		fprintf (stderr, "epoll error\n");
	      		close (revents[ii].data.fd);
	      		continue;
	    	}
			/* check if new connection has arrived */
			if (revents[ii].data.fd == listenfd) {
				connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
				fprintf(stderr, "new connection fd %d from client %s:%d accepted\n", 
						connfd, 
						inet_ntoa(cliaddr.sin_addr), 
						ntohs(cliaddr.sin_port));

				/* add this fd for new events to watch out for */
				event.data.fd = connfd;
				event.events = EPOLLIN;
				status = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event);
				if (status == -1) {
                      fprintf(stderr, "epoll_ctl for fd %d\n", connfd);
                      exit(1);
				}
			} else { /* data from some client on existing connection */
				status = str_echo(revents[ii].data.fd);
				if (status <= 0) {
                  /* close make epoll remove it from fds being monitored */
					fprintf(stderr, "fd %d closed\n", revents[ii].data.fd);
					close (revents[ii].data.fd);
				}
			}
		} /* end for (ii ) */
	} /* for ( ; ; ) */
	close(listenfd);
}

/* just do one read. For subsequent read, this function will be invoked again */
int str_echo(int clientfd) {
	char buf[BUFFSIZE];
	int size;

	bzero(buf, sizeof(buf));
	size = recv(clientfd, buf, sizeof(buf), 0);
	if (size > 0) {
		size = send(clientfd, buf, strnlen(buf, sizeof(buf)), 0);
	}
	return size;
}
