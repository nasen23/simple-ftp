#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>


#define IP_SERVER "127.0.0.1"
#define PORT_SERVER 6789

#define BUF_SIZE 8192
#define IP_MAXLEN 50

#define error(e, x)                  \
    do {                             \
        perror("Error in " #e "\n"); \
        fprintf(stderr, "%d\n", x);  \
        exit(-1);                    \
    } while(0)

int create_socket(char *host, int port, struct sockaddr_in* addr) {
    // this function complete all the steps to create a socket
    // stops before making any connections or binds
    int reuse = 1;
    int x, sockfd;

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
        return -1;
    }

    if ((x = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))) < 0) {
        return -1;
    }

    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    if ( (x = inet_pton(AF_INET, host, &addr->sin_addr)) < 0 ) {
        return -1;
    }

    return sockfd;
}

int get_my_ipaddr(char *addr, struct sockaddr_in *pasv_addr) {
    gethostname(addr, IP_MAXLEN);
    struct hostent *hp = gethostbyname(addr);
    inet_ntop(AF_INET, (struct in_addr*)hp->h_addr_list[0], addr, IP_MAXLEN);

    return 0;
}

int isdigits(char *src) {
    char *p = src;
    if (!p || !*p) return 0;

    for (; *p; ++p) {
        if (!isdigit(*p)) {
            return 0;
        }
    }

    return 1;
}

#endif
