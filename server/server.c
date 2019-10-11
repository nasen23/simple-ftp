#include "../common/common.h"
#include "server.h"
#include "parse_cmd.h"

socklen_t sockaddr_size = sizeof(struct sockaddr);

int main(int argc, char **argv) {
    int n;
    int server_sfd, client_sfd;        //监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in sin_server, sin_client;

    char *basedir = "/tmp";
    chdir(basedir);

    //设置本机的ip和port
    memset(&sin_server, 0, sizeof(sin_server));
    sin_server.sin_family = AF_INET;
    sin_server.sin_port = htons(PORT_SERVER);
    sin_server.sin_addr.s_addr = htonl(INADDR_ANY);    //监听"0.0.0.0"

    //创建socket
    if ((server_sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        error("socket ()", server_sfd);
    }

    int reuse = 1;
    if ((n = setsockopt(server_sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))) < 0) {
        error("setsockopt(SO_REUSEADDR)", n);
    }

    //将本机的ip和port与socket绑定
    if ((n = bind(server_sfd, (struct sockaddr*) &sin_server, sockaddr_size)) < 0) {
        error("bind ()", n);
    }

    //开始监听socket
    if ((n = listen(server_sfd, 10)) < 0) {
        error("listen ()", n);
    }

    printf("Simple FTP server started at %s:%d. Waiting for clients... \n\r\n", IP_SERVER, PORT_SERVER);

    //持续监听连接请求
    while (1) {
        //等待client的连接 -- 阻塞函数
        if ((client_sfd = accept(server_sfd, (struct sockaddr *) &sin_client, &sockaddr_size)) < 0) {
            error("accept ()", client_sfd);
        }

        int pid = fork();
      
        if (pid < 0) {
            error("forking child process", pid);
        } else if (pid == 0) {
            close(server_sfd);
            serve_for_client(client_sfd, basedir, &sin_client);
            return 0;
        }

        close(client_sfd);
    }

    close(server_sfd);
}

void serve_for_client(int client_sfd, char *basedir, struct sockaddr_in *sin_client) {
    int err;
    char buf[BUF_SIZE];
    struct CommandList cmd;
    struct server_ctx context;

    printf("Communacating with %s:%d\r\n", inet_ntoa(sin_client->sin_addr), ntohs(sin_client->sin_port));
    send_msg(client_sfd, "220 Anonymous FTP server ready.\r\n");

    context.cmd_fd = client_sfd;
    context.logged_in = 0;
    context.quit = 0;
    context.pasv_fd = 0;
    while (!context.quit) {
        //榨干socket传来的内容
        recv_msg(client_sfd, buf);

        err = parse_string(&cmd, buf);
        if (err) {
            // error happend when parsing arguments
            handle_parse_error(err, &context);
        } else {
            handle_command(&cmd, &context);
        }
    }

    printf("Closing communacation with %s:%d\r\n", inet_ntoa(sin_client->sin_addr), ntohs(sin_client->sin_port));
    close(client_sfd);
    fflush(stdout);
}

void recv_msg(int client_sfd, char* buf) {
    int res;
    if ((res = recv(client_sfd, buf, BUF_SIZE, 0)) < 0) {
        error("recv ()", res);
    }
    buf[res] = '\0';
}

void send_msg(int client_sfd, char *buf) {
    int res;
    if ((res = send(client_sfd, buf, strlen(buf) + 1, 0)) < 0) {
        error("recv ()", res);
    }
}

void handle_command(struct CommandList *cmd, struct server_ctx* context) {
    switch (cmd->cmd) {
        case USER:
            printf("USER\r\n");
            ftp_user(cmd->arg, context);
            break;
        case PASS:
            printf("PASS\r\n");
            ftp_pass(cmd->arg, context);
            break;
        case PORT:
            printf("PORT\r\n");
            ftp_port(cmd->arg, context);
            break;
        case PASV:
            printf("PASV\r\n");
            ftp_pasv(context);
            break;
        case QUIT:
            printf("QUIT\r\n");
            ftp_quit(context);
            break;
        case ABOR:
            printf("ABOR\r\n");
            ftp_abor(context);
            break;
        case SYST:
            printf("SYST\r\n");
            ftp_syst(context);
            break;
        case TYPE:
            printf("TYPE\r\n");
            ftp_type(cmd->arg, context);
            break;
        case MKD:
            printf("MKD\r\n");
            ftp_mkd(cmd->arg, context);
            break;
        case CWD:
            printf("CWD\r\n");
            ftp_cwd(cmd->arg, context);
            break;
        case CDUP:
            printf("CDUP\r\n");
            ftp_cdup(context);
            break;
        case PWD:
            printf("PWD\r\n");
            ftp_pwd(context);
            break;
        case LIST:
            printf("LIST\r\n");
            ftp_list(context);
            break;
        case RMD:
            printf("RMD\r\n");
            ftp_rmd(cmd->arg, context);
            break;
        case DELE:
            printf("DELE\r\n");
            ftp_dele(cmd->arg, context);
            break;
        case RNFR:
            printf("RNFR\r\n");
            ftp_rnfr(cmd->arg, context);
            break;
        case RNTO:
            printf("RNTO\r\n");
            ftp_rnto(cmd->arg, context);
            break;
        default:
            break;
    }

}

void handle_parse_error(int err, struct server_ctx* context) {
    switch (err) {
        case EMPTY_COMMAND:
            send_msg(context->cmd_fd, "Empty command.\r\n");
            break;
        case UNKNOWN_COMMAND:
            send_msg(context->cmd_fd, "Unknown command.\r\n");
            break;
        case MISSING_ARG:
            send_msg(context->cmd_fd, "Missing argument for command requiring argument.\r\n");
        default:
            // no err
            break;
    }
}

void ftp_user(char *username, struct server_ctx* context) {
    ftp_reset_state(context);

    if ( context->logged_in ) {
        send_msg(context->cmd_fd, "500 You are already logged in\r\n");
        return;
    }

    if ( check_valid_user(username) ) {
        strcpy(context->username, username);
        send_msg(context->cmd_fd, "331 User name okay, need password\r\n");
    } else {
        send_msg(context->cmd_fd, "530 Invalid username\r\n");
    }
}

void ftp_pass(char *password, struct server_ctx* context) {
    ftp_reset_state(context);

    if ( context->logged_in ) {
        send_msg(context->cmd_fd, "500 You are already logged in\r\n");
        return;
    }

    if ( !context->username ) {
        send_msg(context->cmd_fd, "500 Invalid username or password\r\n");
        return;
    }

    if ( check_password(context->username, password) ) {
        context->logged_in = 1;
        char message[100];
        sprintf(message, "230 Welcome, user %s.\r\n", context->username);
        send_msg(context->cmd_fd, message);
    } else {
        send_msg(context->cmd_fd, "530 Wrong password.\r\n");
    }
}

void ftp_port(char *addr, struct server_ctx* context) {
    int args[6];
    int sockfd, port;
    char *template = "%d.%d.%d.%d";
    char host[50];
    struct sockaddr_in addr_in;

    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( parse_addr(addr, args) != 0 ) {
        send_msg(context->cmd_fd, " Syntax error on ip address\r\n");
        return;
    }

    sprintf(host, template, args[0], args[1], args[2], args[3]);
    port = args[4] * 256 + args[5];
    if ( (sockfd = create_socket(host, port, &addr_in)) < 0 ) {
        send_msg(context->cmd_fd, " Error creating socket\r\n");
        return;
    }

    context->pasv_fd = sockfd;
    context->flags |= SERVER_PORT;
    send_msg(context->cmd_fd, " Successfully created data transfer socket\r\n");
}

void ftp_pasv(struct server_ctx* context) {
    int x;
    char addr[50];
    char msg[200];
    int sockfd;
    int port = randport();
    int p1, p2;
    struct sockaddr_in addr_in;

    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    // get ip address of server
    if ( get_my_ipaddr(addr) < 0 ) {
        send_msg(context->cmd_fd, " Failed to get the ip address of server\r\n");
        return;
    }

    printf("%s\r\n", addr);

    // create a socket on port between 20000 and 65535
    if ( (sockfd = create_socket(addr, port, &addr_in)) < 0 ) {
        send_msg(context->cmd_fd, " Error creating socket\r\n");
        return;
    }

    if ( bind(sockfd, (struct sockaddr*) &addr_in, sockaddr_size) < 0 ) {
        send_msg(context->cmd_fd, " Error on binding addr\r\n");
        return;
    }

    if ( (listen(sockfd, 10)) < 0 ) {
        send_msg(context->cmd_fd, " Error on listening addr\r\n");
        return;
    }

    printf("%d\r\n", sockfd);
    context->pasv_fd = sockfd;

    for (char *cur = addr; *cur != 0; ++cur) {
        if ( *cur == '.' ) {
            *cur = ',';
        }
    }

    p1 = port / 256, p2 = port % 256;
    sprintf(msg, "227 Entering Passive Mode (%s,%d,%d)", addr, p1, p2);

    context->flags |= SERVER_PASV;
    send_msg(context->cmd_fd, msg);
}

void ftp_quit(struct server_ctx* context) {
    ftp_reset_state(context);
    context->quit = 1;
    send_msg(context->cmd_fd, "221 Farewell\r\n");
}

void ftp_abor(struct server_ctx* context) {
    ftp_reset_state(context);
    context->quit = 1;
    send_msg(context->cmd_fd, "221 Farewell\r\n");
}

void ftp_syst(struct server_ctx* context) {
    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    send_msg(context->cmd_fd, "215 UNIX Type: L8\r\n");
}

void ftp_type(char *type, struct server_ctx *context) {
    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( strcmp("I", type) ) {
        send_msg(context->cmd_fd, "202 Type not implemented, superflous at this site\r\n");
        return;
    }

    send_msg(context->cmd_fd, "200 Type set to I.\r\n");
}

void ftp_mkd(char *dir, struct server_ctx *context) {
    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    struct stat st = { 0 };
    char message[PATH_MAX];

    if ( stat(dir, &st) == -1 ) {
        mkdir(dir, 0700);
        sprintf(message, "257 \"%s\"\r\n", dir);
        send_msg(context->cmd_fd, message);
    } else {
        send_msg(context->cmd_fd, "550 Creating directory failed\r\n");
    }

}

void ftp_cwd(char *dir, struct server_ctx *context) {
    ftp_reset_state(context);
    
    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( chdir(dir) < 0 ) {
        char msg[500];
        sprintf(msg, "550 %s: No such file or directory\r\n", dir);
        send_msg(context->cmd_fd, msg);
        return;
    }

    send_msg(context->cmd_fd, "250 Okay\r\n");
}

void ftp_cdup(struct server_ctx *context) {
    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( chdir("..") < 0 ) {
        send_msg(context->cmd_fd, "550 in root directory\r\n");
        return;
    }

    send_msg(context->cmd_fd, "250 Okay\r\n");
}

void ftp_pwd(struct server_ctx* context) {
    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    char message[PATH_MAX];
    char dir[PATH_MAX];
    getcwd(dir, sizeof(dir));
    sprintf(message, "212 \"%s\"", dir);

    send_msg(context->cmd_fd, message);
}

void ftp_list(struct server_ctx* context) {
    int datasfd;
    char path[PATH_MAX], dir[PATH_MAX];
    FILE *fp;

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( !ftp_test_flags(context, SERVER_PASV | SERVER_PORT) ) {
        send_msg(context->cmd_fd, " Data socket not created\r\n");
        return;
    }

    if ( (datasfd = accept(context->pasv_fd, NULL, 0)) < 0 ) {
        send_msg(context->cmd_fd, " Error trying to accept connection\r\n");
        return;
    }

    getcwd(dir, sizeof(dir));
    sprintf(path, "/bin/ls -al %s", dir);
    fp = popen(path, "r");
    if (fp == NULL) {
        send_msg(context->cmd_fd, " Error getting list message\r\n");
        return;
    }

    printf("%s", path);
    while ( fgets(path, sizeof(path) - 1, fp) != NULL ) {
        send_msg(datasfd, path);
    }
    close(datasfd);
    pclose(fp);

    send_msg(context->cmd_fd, "200 Ok\r\n");

    ftp_reset_state(context);
}

void ftp_rmd(char *dir, struct server_ctx *context) {
    int rc;

    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( (rc = rmdir(dir)) != 0 ) {
        send_msg(context->cmd_fd, "550 failed to delete directory\r\n");
        return;
    }

    send_msg(context->cmd_fd, "250 Ok\r\n");
}

void ftp_dele(char *fpath, struct server_ctx* context) {
    int rc;

    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( (rc = unlink(fpath)) != 0 ) {
        send_msg(context->cmd_fd, "550 Failed to delete file\r\n");
        return;
    }

    send_msg(context->cmd_fd, "250 Ok\r\n");
}

void ftp_rnfr(char *fpath, struct server_ctx* context) {
    int rc;
    struct stat st;
   
    ftp_reset_state(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    /* make sure file exists in path */
    if ( (rc = lstat(fpath, &st)) != 0 ) {
        send_msg(context->cmd_fd, "450 No such file or directory\r\n");
        return;
    }
    strcpy(context->fname, fpath);

    /* ready for rename action */
    context->flags |= SERVER_RENAME;
    send_msg(context->cmd_fd, "350 Ok\r\n");
}

void ftp_rnto(char *fname, struct server_ctx* context) {
    int rc;

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( !ftp_test_flags(context, SERVER_RENAME) ) {
        send_msg(context->cmd_fd, "503 Bad sequence of commands\r\n");
        return;
    }

    if ( (rc = rename(context->fname, fname)) != 0 ) {
        send_msg(context->cmd_fd, "550 Failed to rename file/directory\r\n");
        return;
    }

    send_msg(context->cmd_fd, "250 Ok\r\n");

    ftp_reset_state(context);
}

void ftp_reset_datasock(struct server_ctx *context) {
    ftp_clear_flags(context, SERVER_PASV | SERVER_PORT);

    close(context->pasv_fd);
    context->pasv_fd = 0;
}

int ftp_test_flags(struct server_ctx* context, int flags) {
    return context->flags & flags;
}

void ftp_clear_flags(struct server_ctx* context, int flags) {
    context->flags &= ~flags;
}

void ftp_reset_state(struct server_ctx* context) {
    if ( ftp_test_flags(context, SERVER_PASV | SERVER_PORT) ) {
        close(context->pasv_fd);
    }

    context->flags = 0;
}

int check_valid_user(char *username) {
    for (int i = 0; i < AUTH_USER_COUNT; ++i) {
        if ( !strcmp(username, auth_users[i]) ) {
            return 1;
        }
    }

    return 0;
}

int check_password(char *username, char *password) {
    // TODO: check password
    return 1;
}

void print_usage() {
    printf("usage: ");
}

int randport() {
    const int minport = 20000, maxport = 65535;
    int d = maxport - minport + 1;
    int r = rand() % d;

    return r + minport;
}
