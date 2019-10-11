#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>


#define IP_SERVER "127.0.0.1"
#define PORT_SERVER 6789

#define BUF_SIZE 8192

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
    addr->sin_port = port;
    if ( (x = inet_pton(AF_INET, host, &addr->sin_addr)) < 0 ) {
        return -1;
    }

    return sockfd;
}

int get_my_ipaddr(char *addr) {
    struct ifaddrs *ifaddr, *ifa;
    int n;

    if ( getifaddrs(&ifaddr) < 0 ) {
        return -1;
    }

    *addr = 0;
    ifa = ifaddr;
    while (ifa) {
        if ( ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET ) {
            struct sockaddr_in *addr_in = (struct sockaddr_in*) ifa->ifa_addr;
            sprintf(addr, "%s", inet_ntoa(addr_in->sin_addr));
            break;
        }
    }

    if ( *addr == 0 ) {
        return -1;
    }

    freeifaddrs(ifaddr);

    return 0;
}

#endif
