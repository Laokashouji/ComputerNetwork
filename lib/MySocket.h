//
// Created by LaoKaShouJi on 2022/7/8.
//

#ifndef COMPUTERNETWORK_MYSOCKET_H
#define COMPUTERNETWORK_MYSOCKET_H
#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include<winSock2.h>
#include<windows.h>
#pragma comment(lib,"ws2_32.lib")
#else
#include<arpa/inet.h>//selcet
	#include<unistd.h>//uni std
	#include<string.h>
	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif

//创建套接字,unBlock为1表示设置为非阻塞模式
SOCKET createSock(int unBlock);

//设置端口和IP地址
void setSockAddr(SOCKADDR_IN *name, int port, char* addr);

//绑定socket
void bindSock(SOCKET sock, SOCKADDR_IN *sockAddr);


#endif //COMPUTERNETWORK_MYSOCKET_H
