#include      "tcpcommon.h"

int
main(int argc, char **argv) {
	int     listenfd, connfd;
  pid_t   childpid;
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;
	char buf[BUFFSIZE];
	int size;

	listenfd = socket (AF_INET, SOCK_STREAM, 0);

	bzero((void *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
	servaddr.sin_port = htons(SERVER_PORT);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

	clilen = sizeof(cliaddr);
	for (; ; ) {
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		bzero(buf, sizeof(buf));
		while ( (size = recv(connfd, buf, sizeof(buf), 0) > 0)) {;
			send(connfd, buf, strnlen(buf, sizeof(buf)), 0);
			bzero(buf, sizeof(buf));
		}
	close(connfd);          
	}

}
