#include "common.h"
#include "server.h"

socklen_t sockaddr_size = sizeof(struct sockaddr);

void serve_for_client(int client_sfd);

int main(int argc, char **argv) {
    int n;
    int server_sfd, client_sfd;        //监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in sin_server, sin_client;

    //创建socket
    if ((server_sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        error("socket ()", server_sfd);
    }

    //设置本机的ip和port
    memset(&sin_server, 0, sizeof(sin_server));
    sin_server.sin_family = AF_INET;
    sin_server.sin_port = PORT_SERVER;
    sin_server.sin_addr.s_addr = htonl(INADDR_ANY);    //监听"0.0.0.0"

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

        printf("Communacating with %s:%d\n", inet_ntoa(sin_client.sin_addr), ntohs(sin_client.sin_port));

        serve_for_client(client_sfd);
    }


    close(server_sfd);
}

void serve_for_client(int client_sfd) {
    int p;
    char buf[BUF_SIZE];

    while (1) {
        //榨干socket传来的内容
        p = 0;
        if ((p = recv(client_sfd, buf, BUF_SIZE, 0)) < 0) {
            error("recv ()", p);
        }
        //socket接收到的字符串并不会添加'\0'
        buf[p] = '\0';

        //字符串处理
        for (p = 0; p < strlen(buf); p++) {
            buf[p] = toupper(buf[p]);
        }

        //发送字符串到socket
        if ((p = send(client_sfd, buf, strlen(buf), 0)) < 0) {
            error("send ()", p);
        }
    }

    close(client_sfd);
    fflush(stdout);
}
