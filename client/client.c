#include "../common/common.h"

#define DATA_SOCK_PORT 25725

int main(int argc, char **argv) {
    int sockfd, datasfd;
    struct sockaddr_in addr, data_addr;
    char data[BUF_SIZE];
    int len;
    int n;

    if ( (sockfd = create_socket(IP_SERVER, PORT_SERVER, &addr)) < 0 ) {
        error("create_socket ()", sockfd);
    }

    if ( (datasfd = create_socket(IP_SERVER, DATA_SOCK_PORT, &data_addr)) < 0 ) {
        error("create_data_socket ()", datasfd);
    }

    int pid = fork();

    if (pid < 0) {
        error("forking child process", pid);
    } else if (pid == 0) {
        // child process listens to data sock always
        if ((n = bind(datasfd, (struct sockaddr*)&data_addr, sizeof(struct sockaddr_in))) < 0) {
            error("data bind ()", n);
        }

        if ( (n = listen(datasfd, 10)) < 0 ) {
            error("listen ()", n);
        }

        while (1) {
            if ((n = recv(datasfd, data, BUF_SIZE, 0)) < 0) {
                error("recv ()", n);
            }

            printf("%s", data);
        }

        close(datasfd);
    } else {
        // parent process listens to control socket
        //连接上目标主机（将socket和目标主机连接）-- 阻塞函数
        if ( (n = connect(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) < 0 ) {
            error("connect ()", n);
        }

        // receive initial message here
        if ( (n = recv(sockfd, data, BUF_SIZE, 0)) < 0 ) {
            error("recv ()", n);
        }

        printf("FROM SERVER: %s", data);

        while (1) {
            //获取键盘输入
            fgets(data, BUF_SIZE, stdin);
            // len = strlen(data);
            // data[len] = '\n';
            // data[len + 1] = '\0';

            //把键盘输入写入socket
            // write函数不保证所有的数据写完，可能中途退出
            if ( (n = send(sockfd, data, strlen(data), 0)) < 0 ) {
              error("send ()", n);
            }

            //榨干socket接收到的内容
            if ( (n = recv(sockfd, data, BUF_SIZE, 0)) < 0 ) {
              error("recv ()", n);
            }

            //注意：read并不会将字符串加上'\0'，需要手动添加
            // data[p - 1] = '\0';

            printf("%s", data);
        }

        close(sockfd);
    }

    return 0;
}
