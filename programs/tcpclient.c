/**
 * on receipt of each new request, server spawns a child and
 * thus each client is handled by a separate child process.
 * Till server exits, the child will exit, but will remain a zombie process
 */

#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* parameters for connecting to server */
#define SERVER_PORT    9999
#define BUFFSIZE        8192         /* buffer size for reads and writes */

#define    SEND_SIZE    1000

int main(int argc, char **argv) {
    int     listenfd, connfd;
    pid_t   childpid;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    int    child_status;
    int opt;
    char *ip_addr = NULL;
    unsigned short port = 0;
    int    msg_cnt = 0;
    int initial_delay = 0;
    int    avg_gap = 1000; /* duration (in ms) between two sends */
    char    buf[BUFFSIZE];
    int    ii;
    int    sock;
    int    bytes_sent = 0;
    int    bytes_recd = 0;
    int    size;
    int    starttime;
    int    endtime;

    while ((opt = getopt(argc, argv, "s:p:c:d:i:")) != -1) {
        switch (opt) {
        case 's':
            ip_addr = optarg;
            break;

        case 'p':
            port = atoi(optarg);
            break;

        case 'c':
            msg_cnt = atoi(optarg);
            break;

        case 'd':
            initial_delay = atoi(optarg);
            break;

        case 'i':
            avg_gap = atoi(optarg);
            break;

        default:
            fprintf(stderr, "Usage: %s -s <ipaddr> -p <port> -c <req_cnt> -d <initial_delay(s)> -i <gap(ms)>\n", argv[0]);
            exit(1);
            break;
        }
    }
    if ((ip_addr == (char *)NULL) || (port == 0) || (msg_cnt == 0)) {
        fprintf(stderr, "Usage: %s -s <ipaddr> -p <port> -c <req_cnt> -d <initial_delay(s)> -i <gap(ms)>\n", argv[0]);
        exit(1);
    }

    if (initial_delay > 0) {
        sleep(initial_delay);
    }
    sock = socket (AF_INET, SOCK_STREAM, 0);
    bzero((void *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_addr);
    servaddr.sin_port = htons(port);

    srandom(getpid()); /* set the seed for random numbers */
    starttime = time(NULL);
    if (connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
        fprintf(stderr, "Error in connecting to server %s at port %hu\n", ip_addr, port);
        exit(1);
    }

    bzero(buf, BUFFSIZE);
    for (ii = 0; ii < msg_cnt; ii++) {
        size = snprintf(buf, BUFFSIZE, "Client pid=%d, The msg number %d, \n", getpid(), ii );
        usleep(random() % (avg_gap*1000)); 
        bytes_sent += send(sock, buf, size, 0);
        bzero(buf, BUFFSIZE);
        bytes_recd += recv(sock, buf, BUFFSIZE, 0);
        printf("Rcvd from Server: %s", buf);
    }
    close(sock);
    endtime = time(NULL);
    printf("pid=%d, time_taken = %d, bytes_sent=%d, bytes_recd=%d\n", getpid(), endtime - starttime, bytes_sent, bytes_recd);
}
