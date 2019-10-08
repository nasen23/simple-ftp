#ifndef __SERVER_H_
#define __SERVER_H_

#include <stddef.h>

struct sockaddr_in;
struct CommandList;

struct ServerCtx {
    int client_sfd;
    int logged_in;
    int quit;
    char *username;
};

const char* auth_users[] = {
"anonymous"
};

const size_t AUTH_USER_COUNT = 1;

void serve_for_client(int client_sfd, struct sockaddr_in *sin_client);
void recv_msg(int client_sfd, char *);
void send_msg(int client_sfd, char *msg);
void handle_command(struct CommandList*, struct ServerCtx*);
void handle_parse_error(int err, struct ServerCtx*);

void ftp_user(char *username, struct ServerCtx*);
void ftp_pass(char *password, struct ServerCtx*);
void ftp_quit(struct ServerCtx*);

int check_valid_user(char *username);
int check_password(char *username, char *password);

void print_usage();



#endif // __SERVER_H_
