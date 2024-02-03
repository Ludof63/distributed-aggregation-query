#include "Server.h"

int Server::startSever(char* port) {

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        perror("Coordinator: socket error");
        return -1;
    }

    struct sockaddr_in socketAddr;
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons((uint16_t)atoi(port));
    socketAddr.sin_addr.s_addr = INADDR_ANY;

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(sockfd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) == -1) {
        perror("Coordinator: bind error");;
        return -1;
    }

    if (listen(sockfd, LISTEN_BACKLOG) == -1) {
        perror("Coordinator: listen error");
        return -1;
    }

    return sockfd;
}
