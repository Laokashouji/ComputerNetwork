//
// Created by LaoKaShouJi on 2022/7/7.
//

#ifndef COMPUTERNETWORK_DATASTRUCTURE_H
#define COMPUTERNETWORK_DATASTRUCTURE_H

#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <time.h>

#define BUFSIZE 1024
#define PORT 53
#define DEF_DNS_ADDRESS "192.168.31.1"
#define LOCAL_DNS_ADDRESS "127.0.0.1"
#define MAX_TABLE_SIZE 1500
#define NOTFOUND 32767
#define DNSHEADER 12
#define MAX_IP4STRING_LENGTH 15
#define MAX_DNS_SIZE 512
#define FULL_DOMAIN_LENGTH 253
#define MAX_CACHE_LENGTH 30
#define LOCAL_DNS_FILE "../doc/dnsrelay.txt"
#define WAIT_TIME 3

//DNS域名解析表结构
typedef struct DNSTransmit {
    char *IP;                        //IP地址
    char *domain;                    //域名
    int time;
} DNSTable;

//ID转换表结构
typedef struct IDTransmit {
    u_short oldID;                      //原有ID
    SOCKADDR_IN client;                 //请求者套接字地址
    BOOL unavailable;                   //标记是否完成解析
} IDTable;
#endif //COMPUTERNETWORK_DATASTRUCTURE_H
