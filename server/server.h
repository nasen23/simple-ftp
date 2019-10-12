#ifndef __SERVER_H_
#define __SERVER_H_

#include <stddef.h>
#include <getopt.h>
#include <limits.h>

struct sockaddr_in;
struct CommandList;

typedef enum {
SERVER_PASV   = 1 << 0, /* pasv addr ready for data transfer cmd */
SERVER_PORT   = 1 << 1, /* port addr ready for data transfer cmd */
SERVER_RENAME = 1 << 2, /* last cmd was rnfr and right path */
} server_flag_t;

struct server_ctx {
    int cmd_fd;
    int pasv_fd;
    int logged_in;
    int quit;
    size_t fpos;

    char username[256];
    char fname[512];
    server_flag_t flags;
};

typedef struct {
    int port;
    char root[PATH_MAX];
} server_opt_t;

const char* auth_users[] = {
"anonymous"
};
const size_t AUTH_USER_COUNT = 1;

static struct option long_options[] = {
{"port", required_argument, NULL, 'p'},
{"root", required_argument, NULL, 'r'},
{NULL, 0, NULL, 0}
};

void serve_for_client(int client_sfd, struct sockaddr_in *sin_client);
void recv_msg(int client_sfd, char *);
void send_msg(int client_sfd, char *msg);
void handle_command(struct CommandList*, struct server_ctx*);
void handle_parse_error(int err, struct server_ctx*);

void ftp_user(char *username, struct server_ctx*);
void ftp_pass(char *password, struct server_ctx*);
void ftp_port(char *addr, struct server_ctx*);
void ftp_pasv(struct server_ctx*);
void ftp_quit(struct server_ctx*);
void ftp_abor(struct server_ctx*);
void ftp_syst(struct server_ctx*);
void ftp_type(char *type, struct server_ctx*);
void ftp_mkd(char *dir, struct server_ctx*);
void ftp_cwd(char *dir, struct server_ctx*);
void ftp_cdup(struct server_ctx*);
void ftp_pwd(struct server_ctx*);
void ftp_list(struct server_ctx*);
void ftp_rmd(char *dir, struct server_ctx*);
void ftp_dele(char *fpath, struct server_ctx*);
void ftp_rnfr(char *fpath, struct server_ctx*);
void ftp_rnto(char *fname, struct server_ctx*);
void ftp_retr(char *fname, struct server_ctx*);
void ftp_stor(char *fname, struct server_ctx*);
void ftp_rest(char *pos, struct server_ctx*);

void ftp_reset_datasock(struct server_ctx*);
int ftp_test_flags(struct server_ctx*, int flags);
void ftp_clear_flags(struct server_ctx*, int flags);
void ftp_reset_state(struct server_ctx*);

int check_valid_user(char *username);
int check_password(char *username, char *password);

void print_usage();

int randport();

#endif // __SERVER_H_
