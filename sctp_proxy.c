#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

#define MAX_BUFFER_SIZE 4096
#define MAX_BACKLOG_SIZE 5

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <local_ip> <local_port> <remote_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 解析命令行参数
    const char* local_ip = argv[1];
    const int local_port = strtol(argv[2], NULL, 10);
    const char* remote_ip = argv[3];

    // 创建本地监听套接字
    struct sockaddr_in local_addr = { 0 };
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    inet_pton(AF_INET, local_ip, &local_addr.sin_addr);

    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (bind(listen_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, MAX_BACKLOG_SIZE) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("Listening on %s:%d...\n", local_ip, local_port);

    while (1) {
        // 接受连接请求
        struct sockaddr_in client_addr = { 0 };
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Accepted connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 创建远程连接套接字
        struct sockaddr_in remote_addr = { 0 };
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(local_port);
        inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr);

        int remote_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (remote_fd < 0) {
            perror("socket");
            return EXIT_FAILURE;
        }

        if (connect(remote_fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
            perror("connect");
            return EXIT_FAILURE;
        }

        // 开始转发数据
        char buffer[MAX_BUFFER_SIZE];
        ssize_t n;

        while ((n = recv(client_fd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
            if (send(remote_fd, buffer, n, 0) < 0) {
                perror("send");
                break;
            }
        }

        close(remote_fd);
        close(client_fd);

        printf("Connection closed.\n");
    }

    close(listen_fd);

    return EXIT_SUCCESS;
}
