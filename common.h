#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>


#define IP_SERVER "127.0.0.1"
#define PORT_SERVER 6791

#define BUF_SIZE 8192

#define error(e, x) \
    do {        \
        perror("Error in " #e "\n"); \
        fprintf(stderr, "%d\n", x); \
        exit(-1); \
    } while(0)


#endif
