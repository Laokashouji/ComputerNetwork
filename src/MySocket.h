//
// Created by LaoKaShouJi on 2022/7/8.
//

#ifndef COMPUTERNETWORK_MYSOCKET_H
#define COMPUTERNETWORK_MYSOCKET_H
#pragma once

#include <WinSock2.h>

//创建套接字,unBlock为1表示设置为非阻塞模式
SOCKET createSock(int unBlock);

//设置端口和IP地址
void setSockAddr(SOCKADDR_IN *name, int port, char* addr);

//绑定socket
void bindSock(SOCKET sock, SOCKADDR_IN *sockAddr);


#endif //COMPUTERNETWORK_MYSOCKET_H
