//
// Created by LaoKaShouJi on 2022/7/8.
//

#include "MySocket.h"
#include <string.h>
#include <stdio.h>

SOCKET createSock(int unBlock) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    //设置为非阻塞模式
    if(unBlock == 1) {
        ioctlsocket(sock, FIONBIO, (u_long FAR *) &unBlock);//将外部套街口设置为非阻塞
    }
    return sock;
}

void setSockAddr(SOCKADDR_IN *name, int port, char* addr) {
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    name->sin_addr.s_addr = inet_addr(addr);
}

void bindSock(SOCKET sock, SOCKADDR_IN *sockAddr) {
    if (bind(sock, (SOCKADDR*) sockAddr, sizeof(*sockAddr)))
    {
        printf("Bind 53 port failed.\n");
        exit(-1);
    }
    else
        printf("Bind 53 port success.\n");
}



