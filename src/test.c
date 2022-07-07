//
// Created by LaoKaShouJi on 2022/7/6.
//
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define N 128

int main(int argc, char const *argv[])
{
    if(argc < 3)
    {
        perror("./a.out ip port");
        exit(1);
    }

    printf("%s\n", argv[1]);
    printf("%s\n", argv[2]);

    WSADATA wsd;
    WSAStartup(MAKEWORD(2,2),&wsd);
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("failed to create a socket");
        exit(1);
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[2]));
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1)
    {
        perror("failed to bind socket");
        exit(1);
    }

    char buf[N] = "";
    struct sockaddr_in clientaddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    while(1)
    {
        if(recvfrom(sockfd, buf, N, 0, (struct sockaddr *) &clientaddr, &addrlen) == -1)
        {
            perror("failed to receive socket message");
            exit(1);
        }
        else {
            printf("ip:%s, port:%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
            printf("from client:%s\n", buf);
        }
    }

    WSACleanup();
    return 0;
}