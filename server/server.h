#ifndef __SERVER_H_
#define __SERVER_H_

#include <stddef.h>

struct sockaddr_in;
struct CommandList;

struct ServerCtx {
    int client_sfd;
    int logged_in;
    int quit;
    int datasfd;
    char *username;
    char *dir;
};

const char* auth_users[] = {
"anonymous"
};

const size_t AUTH_USER_COUNT = 1;

void serve_for_client(int client_sfd, char *basedir, struct sockaddr_in *sin_client);
void recv_msg(int client_sfd, char *);
void send_msg(int client_sfd, char *msg);
void handle_command(struct CommandList*, struct ServerCtx*);
void handle_parse_error(int err, struct ServerCtx*);

void ftp_user(char *username, struct ServerCtx*);
void ftp_pass(char *password, struct ServerCtx*);
void ftp_port(char *addr, struct ServerCtx*);
void ftp_pasv(struct ServerCtx*);

void ftp_quit(struct ServerCtx*);
void ftp_abor(struct ServerCtx*);
void ftp_syst(struct ServerCtx*);
void ftp_type(char *type, struct ServerCtx*);
void ftp_pwd(struct ServerCtx*);

int check_valid_user(char *username);
int check_password(char *username, char *password);

void print_usage();

int randport();

#endif // __SERVER_H_
