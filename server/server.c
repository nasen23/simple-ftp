#include "../common/common.h"
#include "server.h"
#include "parse_cmd.h"

socklen_t sockaddr_size = sizeof(struct sockaddr);

int main(int argc, char **argv) {
    int n;
    int server_sfd, client_sfd;        //监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in sin_server, sin_client;

    char *basedir = "/tmp";

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

    printf("Simple FTP server started at %s:%d. Waiting for clients... \n\n", IP_SERVER, PORT_SERVER);

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
    struct ServerCtx context;

    printf("Communacating with %s:%d\n", inet_ntoa(sin_client->sin_addr), ntohs(sin_client->sin_port));
    send_msg(client_sfd, "220 Anonymous FTP server ready.\n");

    context.client_sfd = client_sfd;
    context.logged_in = 0;
    context.quit = 0;
    context.username = NULL;
    context.datasfd = 0;
    context.dir = basedir;
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

    free(context.username);

    printf("Closing communacation with %s:%d\n", inet_ntoa(sin_client->sin_addr), ntohs(sin_client->sin_port));
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

void handle_command(struct CommandList *cmd, struct ServerCtx* context) {
    switch (cmd->cmd) {
        case USER:
            printf("USER");
            ftp_user(cmd->arg, context);
            break;
        case PASS:
            printf("PASS");
            ftp_pass(cmd->arg, context);
            break;
        case PORT:
            printf("PORT");
            ftp_port(cmd->arg, context);
            break;
        case PASV:
            printf("PASV");
            ftp_pasv(context);
            break;
        case QUIT:
            printf("QUIT");
            ftp_quit(context);
            break;
        case ABOR:
            printf("ABOR");
            ftp_abor(context);
            break;
        case SYST:
            printf("SYST");
            ftp_syst(context);
            break;
        case TYPE:
            printf("TYPE");
            ftp_type(cmd->arg, context);
            break;
        case PWD:
            printf("PWD");
            ftp_pwd(context);
            break;
        default:
            break;
    }
}

void handle_parse_error(int err, struct ServerCtx* context) {
    switch (err) {
        case EMPTY_COMMAND:
            send_msg(context->client_sfd, "Empty command.\n");
            break;
        case UNKNOWN_COMMAND:
            send_msg(context->client_sfd, "Unknown command.\n");
            break;
        case MISSING_ARG:
            send_msg(context->client_sfd, "Missing argument for command requiring argument.\n");
        default:
            // no err
            break;
    }
}

void ftp_user(char *username, struct ServerCtx* context) {
    if ( context->logged_in ) {
        send_msg(context->client_sfd, "500 You are already logged in\n");
        return;
    }

    if ( check_valid_user(username) ) {
        context->username = realloc(context->username, (1 + strlen(username)) * sizeof(char));

        strcpy(context->username, username);
        send_msg(context->client_sfd, "331 User name okay, need password\n");
    } else {
        send_msg(context->client_sfd, "530 Invalid username\n");
    }
}

void ftp_pass(char *password, struct ServerCtx* context) {
    if ( context->logged_in ) {
        send_msg(context->client_sfd, "500 You are already logged in\n");
        return;
    }

    if ( !context->username ) {
        send_msg(context->client_sfd, "500 Invalid username or password\n");
        return;
    }

    if ( check_password(context->username, password) ) {
        context->logged_in = 1;
        char message[100];
        sprintf(message, "230 Welcome, user %s.\n", context->username);
        send_msg(context->client_sfd, message);
    } else {
        send_msg(context->client_sfd, "530 Wrong password.\n");
    }
}

void ftp_port(char *addr, struct ServerCtx* context) {
    int args[6];
    int sockfd, port;
    char *template = "%d.%d.%d.%d";
    char host[50];
    struct sockaddr_in addr_in;

    if ( !context->logged_in ) {
        send_msg(context->client_sfd, "530 Please login first\n");
        return;
    }

    if ( parse_addr(addr, args) != 0 ) {
        send_msg(context->client_sfd, " Syntax error on ip address\n");
        return;
    }

    sprintf(host, template, args[0], args[1], args[2], args[3]);
    port = args[4] * 256 + args[5];
    if ( (sockfd = create_socket(host, port, &addr_in)) < 0 ) {
        send_msg(context->client_sfd, " Error creating socket\n");
        return;
    }

    context->datasfd = sockfd;
    send_msg(context->client_sfd, " Successfully created data transfer socket\n");
}

void ftp_pasv(struct ServerCtx* context) {
    int x;
    char addr[50];
    char msg[200];
    int sockfd;
    int port = randport();
    int p1, p2;
    struct sockaddr_in addr_in;

    if ( !context->logged_in ) {
        send_msg(context->client_sfd, "530 Please login first\n");
        return;
    }

    // get ip address of server
    if ( get_my_ipaddr(addr) < 0 ) {
        send_msg(context->client_sfd, " Failed to get the ip address of server\n");
        return;
    }

    // create a socket on port between 20000 and 65535
    if ( (sockfd = create_socket(addr, port, &addr_in)) < 0 ) {
        send_msg(context->client_sfd, " Error creating socket\n");
        return;
    }

    context->datasfd = sockfd;

    for (char *cur = addr; *cur != 0; ++cur) {
        if ( *cur == '.' ) {
            *cur = ',';
        }
    }

    p1 = port / 256, p2 = port % 256;
    char *temp = "227 Entering Passive Mode (%s,%d,%d)";
    sprintf(msg, temp, addr, p1, p2);

    send_msg(context->client_sfd, msg);
}

void ftp_quit(struct ServerCtx* context) {
    context->quit = 1;
    send_msg(context->client_sfd, "221 Farewell\n");
}

void ftp_abor(struct ServerCtx* context) {
    context->quit = 1;
    send_msg(context->client_sfd, "221 Farewell\n");
}

void ftp_syst(struct ServerCtx* context) {
    if ( !context->logged_in ) {
        send_msg(context->client_sfd, "530 Please login first\n");
        return;
    }

    send_msg(context->client_sfd, "215 UNIX Type: L8\n");
}

void ftp_type(char *type, struct ServerCtx *context) {
    if ( !context->logged_in ) {
        send_msg(context->client_sfd, "530 Please login first\n");
        return;
    }

    if ( strcmp("I", type) ) {
        send_msg(context->client_sfd, "202 Type not implemented, superflous at this site\n");
        return;
    }

    send_msg(context->client_sfd, "200 Type set to I.\n");
}

void ftp_pwd(struct ServerCtx* context) {
    if ( !context->logged_in ) {
        send_msg(context->client_sfd, "530 Please login first\n");
        return;
    }

    char *template = "212 \"%s\"";
    char message[200];
    sprintf(message, template, context->dir);

    send_msg(context->client_sfd, message);
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
