#include "common.h"

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in addr;
    char data[BUF_SIZE];
    int len;
    int n;

    //创建socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        error("socket ()", sockfd);
    }

    //设置目标主机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = PORT_SERVER;
    if ((n = inet_pton(AF_INET, IP_SERVER, &addr.sin_addr)) < 0) {            //转换ip地址:点分十进制-->二进制
        error("inet_pton ()", n);
    }

    //连接上目标主机（将socket和目标主机连接）-- 阻塞函数
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        error("connect ()", n);
    }

    while (1) {
        //获取键盘输入
        fgets(data, BUF_SIZE, stdin);
        // len = strlen(data);
        // data[len] = '\n';
        // data[len + 1] = '\0';

        //把键盘输入写入socket
        //write函数不保证所有的数据写完，可能中途退出
        if ((n = send(sockfd, data, strlen(data), 0)) < 0) {
            error("send ()", n);
        }

        //榨干socket接收到的内容
        if ((n = recv(sockfd, data, BUF_SIZE, 0)) < 0) {
            error("recv ()", n);
        }

        //注意：read并不会将字符串加上'\0'，需要手动添加
        // data[p - 1] = '\0';

        printf("FROM SERVER: %s", data);

    }
    close(sockfd);

    return 0;
}
