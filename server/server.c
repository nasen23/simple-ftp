#include "common.h"
#include "server.h"
#include "parse_cmd.h"

socklen_t sockaddr_size = sizeof(struct sockaddr);

int main(int argc, char **argv) {
    int n;
    int server_sfd, client_sfd;        //监听socket和连接socket不一样，后者用于数据传输
    int ifchdir = 0;
    char ch;
    struct sockaddr_in sin_server, sin_client;
    server_opt_t opt;

    opt.port = 21;
    strcpy(opt.root, "/tmp");

    while ( (ch = getopt_long_only(argc, argv, "", long_options, NULL)) != -1 ) {
        switch (ch) {
            case 'p':
                if (isdigits(optarg)) {
                    opt.port = atoi(optarg);
                } else {
                    print_usage();
                    return 0;
                }
                break;
            case 'r':
                if ( (n = chdir(optarg)) == 0 ) {
                    ifchdir = 1;
                    strcpy(opt.root, optarg);
                } else {
                    print_usage();
                    return 0;
                }
                break;
        }
    }
    if (!ifchdir) {
        chdir(opt.root);
    }

    //设置本机的ip和port
    memset(&sin_server, 0, sizeof(sin_server));
    sin_server.sin_family = AF_INET;
    sin_server.sin_port = htons(opt.port);
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

    printf("Simple FTP server started at %s:%d. Waiting for clients... \n\r\n", IP_SERVER, opt.port);

    //持续监听连接请求
    while (1) {
        //等待client的连接 -- 阻塞函数
        if ((client_sfd = accept(server_sfd, (struct sockaddr *) &sin_client, &sockaddr_size)) < 0) {
            error("accept ()", client_sfd);
        }

        int pid = fork();
        /* int pid = 0; */
      
        if (pid < 0) {
            error("forking child process", pid);
        } else if (pid == 0) {
            close(server_sfd);
            serve_for_client(client_sfd, &sin_client);
            return 0;
        }

        close(client_sfd);
    }

    close(server_sfd);
}

void serve_for_client(int client_sfd, struct sockaddr_in *sin_client) {
    int err;
    char buf[BUF_SIZE];
    struct CommandList cmd;
    struct server_ctx context;

    srand(time(NULL) % getpid());
    printf("Communacating with %s:%d\r\n", inet_ntoa(sin_client->sin_addr), ntohs(sin_client->sin_port));
    send_msg(client_sfd, "220 Anonymous FTP server ready.\r\n");

    context.cmd_fd = client_sfd;
    context.logged_in = 0;
    context.quit = 0;
    context.pasv_fd = 0;
    context.fpos = 0;
    while (!context.quit && recv_msg(client_sfd, buf)) {
        //榨干socket传来的内容

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

int recv_msg(int sockfd, char* buf) {
    int res;
    if ((res = recv(sockfd, buf, BUF_SIZE - 1, 0)) < 0) {
        error("recv ()", res);
    }
    buf[res] = 0;

    return res;
}

int send_msg(int sockfd, char *buf) {
    int res;
    if ((res = send(sockfd, buf, strlen(buf), 0)) < 0) {
        error("send ()", res);
    }

    return res;
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
        case RETR:
            printf("RETR\r\n");
            ftp_retr(cmd->arg, context);
            break;
        case STOR:
            printf("STOR\r\n");
            ftp_stor(cmd->arg, context);
            break;
        case REST:
            printf("REST\r\n");
            ftp_rest(cmd->arg, context);
            break;
        default:
            break;
    }

}

void handle_parse_error(int err, struct server_ctx* context) {
    switch (err) {
        case EMPTY_COMMAND:
            send_msg(context->cmd_fd, "503 Empty command.\r\n");
            break;
        case UNKNOWN_COMMAND:
            send_msg(context->cmd_fd, "503 Unknown command.\r\n");
            break;
        case MISSING_ARG:
            send_msg(context->cmd_fd, "501 Missing argument for command requiring argument.\r\n");
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
        char message[BUF_SIZE];
        sprintf(message, "230 Welcome, user %s.\r\n", context->username);
        send_msg(context->cmd_fd, message);
    } else {
        send_msg(context->cmd_fd, "530 Wrong password.\r\n");
    }
}

void ftp_port(char *addr, struct server_ctx* context) {
    int args[6];
    int sockfd;
    char *template = "%d.%d.%d.%d";
    struct sockaddr_in addr_in;

    ftp_reset_state(context);
    ftp_reset_datasock(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( parse_addr(addr, args) != 0 ) {
        send_msg(context->cmd_fd, "501 Syntax error on ip address\r\n");
        return;
    }

    sprintf(context->dhost, template, args[0], args[1], args[2], args[3]);
    context->dport = args[4] * 256 + args[5];
    if ( (sockfd = create_socket(context->dhost, context->dport, &addr_in)) < 0 ) {
        send_msg(context->cmd_fd, "425 Error creating socket\r\n");
        return;
    }

    context->pasv_fd = sockfd;
    context->flags |= SERVER_PORT;
    send_msg(context->cmd_fd, "250 Successfully created data transfer socket\r\n");
}

void ftp_pasv(struct server_ctx* context) {
    char addr[IP_MAXLEN];
    char msg[200];
    int sockfd;
    int port = randport();
    int p1, p2;
    struct sockaddr_in pasv_addr;

    printf("port: %d\n", port);

    ftp_reset_state(context);
    ftp_reset_datasock(context);

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    // create a socket on port between 20000 and 65535
    if ( (sockfd = create_socket(addr, port, &pasv_addr)) < 0 ) {
        send_msg(context->cmd_fd, "425 Error creating socket\r\n");
        return;
    }

    if ( bind(sockfd, (struct sockaddr*) &pasv_addr, sockaddr_size) < 0 ) {
        send_msg(context->cmd_fd, "425 Error on binding addr\r\n");
        return;
    }

    if ( (listen(sockfd, 10)) < 0 ) {
        send_msg(context->cmd_fd, "425 Error on listening addr\r\n");
        return;
    }

    printf("%d\r\n", sockfd);
    context->pasv_fd = sockfd;

    if ( get_my_ipaddr(addr, &pasv_addr) < 0 ) {
        send_msg(context->cmd_fd, "550 Error getting ip addr\r\n");
        return;
    }

    for (char *cur = addr; *cur; ++cur) {
        if ( *cur == '.' ) {
            *cur = ',';
        }
    }

    p1 = port / 256, p2 = port % 256;
    sprintf(msg, "227 Entering Passive Mode (%s,%d,%d)\r\n", addr, p1, p2);

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

    char message[BUF_SIZE];
    char dir[PATH_MAX];
    getcwd(dir, sizeof(dir));
    sprintf(message, "257 \"%s\"\r\n", dir);

    send_msg(context->cmd_fd, message);
}

void ftp_list(struct server_ctx* context) {
    int datasfd;
    char path[BUF_SIZE], dir[PATH_MAX];
    FILE *fp;
    struct sockaddr_in dest_addr;

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    send_msg(context->cmd_fd, "150 Ready for file transfer\r\n");

    if ( !ftp_test_flags(context, SERVER_PASV | SERVER_PORT) ) {
        send_msg(context->cmd_fd, "425 Data socket not created\r\n");
        return;
    }

    if ( ftp_test_flags(context, SERVER_PASV) ) {
        if ( (datasfd = accept(context->pasv_fd, NULL, 0)) < 0 ) {
            send_msg(context->cmd_fd, "426 Error trying to accept connection\r\n");
            return;
        }
    } else {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(context->dport);
        dest_addr.sin_addr.s_addr = inet_addr(context->dhost);
        if ( connect(context->pasv_fd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr)) < 0 ) {
            send_msg(context->cmd_fd, "426 Error trying to connect\r\n");
            return;
        }
        datasfd = context->pasv_fd;
    }

    getcwd(dir, sizeof(dir));
    sprintf(path, "/bin/ls -al \"%s\"", dir);
    fp = popen(path, "r");
    if (fp == NULL) {
        send_msg(context->cmd_fd, "451 Error getting list message\r\n");
        return;
    }

    while ( fgets(path, sizeof(path) - 1, fp) != NULL ) {
        send_msg(datasfd, path);
    }
    close(datasfd);
    pclose(fp);

    send_msg(context->cmd_fd, "200 Ok\r\n");

    ftp_reset_state(context);
    ftp_reset_datasock(context);
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

void ftp_retr(char *fname, struct server_ctx* context) {
    int datasfd;
    struct stat fst;
    int sent, remain;
    int fd;
    struct sockaddr_in dest_addr;

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    send_msg(context->cmd_fd, "150 Ready for file transfer\r\n");

    if ( !ftp_test_flags(context, SERVER_PASV | SERVER_PORT) ) {
        send_msg(context->cmd_fd, "425 Data socket not created\r\n");
        return;
    }

    if ( ftp_test_flags(context, SERVER_PASV) ) {
        if ( (datasfd = accept(context->pasv_fd, NULL, 0)) < 0 ) {
            send_msg(context->cmd_fd, "426 Error trying to accept connection\r\n");
            return;
        }
    } else {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(context->dport);
        dest_addr.sin_addr.s_addr = inet_addr(context->dhost);
        if ( connect(context->pasv_fd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr)) < 0 ) {
            send_msg(context->cmd_fd, "426 Error trying to connect\r\n");
            return;
        }
        datasfd = context->pasv_fd;
    }

    fd = open(fname, O_RDONLY);
    if (fd == -1) {
        send_msg(context->cmd_fd, "550 Error reading file\r\n");
        return;
    }

    if ( fstat(fd, &fst) ) {
        send_msg(context->cmd_fd, "550 Error getting status of file\r\n");
        return;
    }

    remain = fst.st_size;

    if ( context->fpos ) {
        lseek(fd, context->fpos, SEEK_SET);
    }

    while ( (sent = sendfile(datasfd, fd, NULL, BUFSIZ)) > 0 && remain > 0 ) {
        remain -= sent;
    }

    close(datasfd);

    send_msg(context->cmd_fd, "226 Ok\r\n");

    ftp_reset_state(context);
    ftp_reset_datasock(context);
    context->fpos = 0; // reset rest command state
}

void ftp_stor(char *fname, struct server_ctx* context) {
    int datasfd;
    char line[BUF_SIZE];
    FILE *fp;
    int receive = 0;
    struct sockaddr_in dest_addr;

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    send_msg(context->cmd_fd, "150 Ready for file transfer\r\n");

    if ( !ftp_test_flags(context, SERVER_PASV | SERVER_PORT) ) {
        send_msg(context->cmd_fd, "425 Data socket not created\r\n");
        return;
    }

    if ( ftp_test_flags(context, SERVER_PASV) ) {
        if ( (datasfd = accept(context->pasv_fd, NULL, 0)) < 0 ) {
            send_msg(context->cmd_fd, "426 Error trying to accept connection\r\n");
            return;
        }
    } else {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(context->dport);
        dest_addr.sin_addr.s_addr = inet_addr(context->dhost);
        if ( connect(context->pasv_fd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr)) < 0 ) {
            send_msg(context->cmd_fd, "426 Error trying to connect\r\n");
            return;
        }
        datasfd = context->pasv_fd;
    }

    fp = fopen(fname, "wb");
    if (fp == NULL) {
        send_msg(context->cmd_fd, "550 Error opening file\r\n");
        return;
    }

    while (1) {
        receive = recv_msg(datasfd, line);
        if ( receive <= 0 ) {
            break;
        }
        fwrite(line, sizeof(char), receive, fp);
    }

    close(datasfd);
    fclose(fp);

    send_msg(context->cmd_fd, "226 Ok\r\n");
    ftp_reset_state(context);
    ftp_reset_datasock(context);
}

void ftp_rest(char *pos, struct server_ctx* context) {
    ftp_reset_state(context);
    context->fpos = 0;

    if ( !context->logged_in ) {
        send_msg(context->cmd_fd, "530 Please login first\r\n");
        return;
    }

    if ( !isdigits(pos) ) {
        send_msg(context->cmd_fd, "504 Invalid argument\r\n");
        return;
    }

    context->fpos = atoi(pos);

    send_msg(context->cmd_fd, "200 Ok\r\n");
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
    ftp_clear_flags(context, SERVER_RENAME);
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
    // any password is okay
    return 1;
}

void print_usage() {
    printf("usage: [-port n] [-root /path/to/file/area]");
}

int randport() {
    const int minport = 20000, maxport = 65535;
    int d = maxport - minport + 1;
    int r = rand() % d;

    return r + minport;
}
