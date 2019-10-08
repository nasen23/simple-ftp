#include "common.h"
#include "server.h"

socklen_t sockaddr_size = sizeof(struct sockaddr);

int main(int argc, char **argv) {
    int n;
    int server_sfd, client_sfd;        //监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in sin_server, sin_client;

    //设置本机的ip和port
    memset(&sin_server, 0, sizeof(sin_server));
    sin_server.sin_family = AF_INET;
    sin_server.sin_port = PORT_SERVER;
    sin_server.sin_addr.s_addr = htonl(INADDR_ANY);    //监听"0.0.0.0"

    //创建socket
    if ((server_sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        error("socket ()", server_sfd);
    }

    int enable = 1;
    if ((n = setsockopt(server_sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) < 0) {
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
            printf("Communacating with %s:%d\n", inet_ntoa(sin_client.sin_addr), ntohs(sin_client.sin_port));
            serve_for_client(client_sfd);
            return 0;
        }

        close(client_sfd);
    }


    close(server_sfd);
}

void serve_for_client(int client_sfd) {
    int err;
    char buf[BUF_SIZE];
    struct CommandList cmd;
    struct ServerCtx context;

    context.client_sfd = client_sfd;
    context.logged_in = 0;
    context.quit = 0;
    context.username = NULL;
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

    printf("Closing communication with sockfd %d", client_sfd);
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
            ftp_user(cmd->arg, context);
            break;
        case PASS:
            ftp_pass(cmd->arg, context);
            break;
        case QUIT:
            ftp_quit(context);
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
        send_msg(context->client_sfd, "You are already logged in.\n");
        return;
    }

    if ( check_valid_user(username) ) {
        context->username = realloc(context->username, (1 + strlen(username)) * sizeof(char));

        strcpy(context->username, username);
        send_msg(context->client_sfd, "Please input password.\n");
    } else {
        send_msg(context->client_sfd, "Invalid username.\n");
    }
}

void ftp_pass(char *password, struct ServerCtx* context) {
    if ( context->logged_in ) {
        send_msg(context->client_sfd, "You are already logged in.\n");
        return;
    }

    if ( !context->username ) {
        send_msg(context->client_sfd, "I don't know which user you are.\n");
        return;
    }

    if ( check_password(context->username, password) ) {
        context->logged_in = 1;
        char message[100];
        sprintf(message, "Welcome, user %s.\n", context->username);
        send_msg(context->client_sfd, message);
    } else {
        send_msg(context->client_sfd, "Wrong password.\n");
    }
}

void ftp_quit(struct ServerCtx* context) {
    context->quit = 1;
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
