#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 4096
#define GTP_UDP_PORT 2152

struct gtpv1_header {
    uint8_t flags;
    uint8_t message_type;
    uint16_t message_length;
    uint32_t teid;
};

void handle_gtp_packet(int fd, struct sockaddr_in* client_addr) {
    char buffer[MAX_BUFFER_SIZE];
    ssize_t n = recvfrom(fd, buffer, MAX_BUFFER_SIZE, 0, NULL, NULL);
    if (n < 0) {
        perror("recvfrom");
        return;
    }

    // 解析GTP头部
    struct gtpv1_header* gtp_hdr = (struct gtpv1_header*)buffer;

    // 将IP地址和UDP端口替换为目标地址和端口
    struct iphdr* ip_hdr = (struct iphdr*)(buffer + sizeof(struct udphdr) + sizeof(struct gtpv1_header));
    struct udphdr* udp_hdr = (struct udphdr*)(buffer + sizeof(struct gtpv1_header));

    ip_hdr->daddr = inet_addr("10.0.0.2");
    udp_hdr->dest = htons(GTP_UDP_PORT);

    // 发送修改后的数据包到目标服务器
    if (sendto(fd, buffer, n, 0, (struct sockaddr*)client_addr, sizeof(*client_addr)) < 0) {
        perror("sendto");
        return;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <listen_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 创建监听套接字
    struct sockaddr_in listen_addr = { 0 };
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(GTP_UDP_PORT);
    inet_pton(AF_INET, argv[1], &listen_addr.sin_addr);

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (bind(fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    printf("Listening on %s:%d...\n", argv[1], GTP_UDP_PORT);

    while (1) {
        struct sockaddr_in client_addr = { 0 };
        socklen_t client_addr_len = sizeof(client_addr);

        // 接收来自客户端的数据包并处理
        handle_gtp_packet(fd, &client_addr);
    }

    close(fd);

    return EXIT_SUCCESS;
}
