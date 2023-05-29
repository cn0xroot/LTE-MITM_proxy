#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define SOURCE_TEID 123456789 // 原始TEID
#define DESTINATION_TEID 987654321 // 目标TEID

int main(int argc, char *argv[]) {
    if (argc != 3) { // 需要指定源IP地址和目标IP地址作为命令行参数
        printf("Usage: %s source_ip_address destination_ip_address\n", argv[0]);
        return -1;
    }

    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // 创建服务器套接字
    if (server_socket_fd == -1) {
        perror("Error creating server socket");
        return -1;
    }

    struct sockaddr_in server_address, client_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]); // 设置服务器IP地址
    server_address.sin_port = htons(8080); // 设置服务器监听端口号

    // 绑定服务器套接字到服务器地址
    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket to address");
        close(server_socket_fd);
        return -1;
    }

    // 监听客户端连接
    if (listen(server_socket_fd, 5) == -1) {
        perror("Error listening for clients");
        close(server_socket_fd);
        return -1;
    }

    printf("TEID Proxy server started\n");

    while (true) { // 循环监听客户端连接请求
        socklen_t client_address_len = sizeof(client_address);
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_address_len); // 接受客户端连接
        if (client_socket_fd == -1) {
            perror("Error accepting client connection");
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE, 0); // 接收客户端数据
        if (bytes_received == -1) {
            perror("Error receiving data from client");
            close(client_socket_fd);
            continue;
        }

        printf("Received %ld bytes of data from client\n", bytes_received);

        // 修改TEID
        memcpy(buffer + 8, &SOURCE_TEID, sizeof(SOURCE_TEID));
        memcpy(buffer + 12, &DESTINATION_TEID, sizeof(DESTINATION_TEID));

        // 转发数据包到目标地址
        struct sockaddr_in target_address;
        memset(&target_address, 0, sizeof(target_address));
        target_address.sin_family = AF_INET;
        target_address.sin_addr.s_addr = inet_addr(argv[2]); // 设置目标IP地址
        target_address.sin_port = htons(8080); // 设置目标端口号

        int target_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // 创建目标套接字
        if (target_socket_fd == -1) {
            perror("Error creating target socket");
            close(client_socket_fd);
            continue;
        }

        // 连接到目标地址
        if (connect(target_socket_fd, (struct sockaddr *)&target_address, sizeof(target_address)) == -1) {
            perror("Error connecting to target address");
            close(client_socket_fd);
            close(target_socket_fd);
            continue;
        }

        ssize_t bytes_sent = send(target_socket_fd, buffer, bytes_received, 0); // 向目标地址发送数据
        if (bytes_sent == -1) {
            perror("Error sending data to target");
            close(client_socket_fd);
            close(target_socket_fd);
            continue;
        }

        printf("Sent %ld bytes of data to target\n", bytes_sent);

        // 接收从目标地址返回的数据
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(target_socket_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1) {
            perror("Error receiving data from target");
            close(client_socket_fd
