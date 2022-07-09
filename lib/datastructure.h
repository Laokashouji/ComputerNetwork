//
// Created by LaoKaShouJi on 2022/7/7.
//

#ifndef COMPUTERNETWORK_DATASTRUCTURE_H
#define COMPUTERNETWORK_DATASTRUCTURE_H

#pragma once

#include <WinSock2.h>
#include <windows.h>
#include <time.h>

#define BUFSIZE 1024 //最大报文缓存大小
#define PORT 53   //53端口号
#define DEF_DNS_ADDRESS "192.168.31.1"	//ipconfig/all 得知外部服务器dns地址
#define LOCAL_DNS_ADDRESS "127.0.0.1" //本地DNS服务器地址
#define MAX_TABLE_SIZE 1500//最大ID转换表大小
#define NOTFOUND 32767 //没有找到
#define ISFOUND 1
#define DNSHEADER 12
#define IP4_LENGTH 4
#define MAX_IP4STRING_LENGTH 15
#define DOMAIN_LENGTH 64 //0~63
#define FULL_DOMAIN_LENGTH 253
#define LOCAL_DNS_FILE "E:\\Codefield\\C\\ComputerNetwork\\doc\\dnsrelay.txt"
//DNS报文首部 12字节
typedef struct DNSHeader
{
    u_short ID; //标志
    u_short Flags; //标识
    u_short QuestionNum;  //问题数
    u_short AnswerNum; //资源记录数
    u_short AuthorNum; //授权资源记录数
    u_short AdditionNum; //额外资源记录数
} DNSHDR;

//DNS域名解析表结构
typedef struct translate
{
    char * IP;						//IP地址
    char * domain;					//域名
} Translate;

//ID转换表结构
typedef struct IDChange
{
    u_short oldID;			//原有ID
    BOOL done;						//标记是否完成解析
    SOCKADDR_IN client;				//请求者套接字地址
} IDTransform;


#endif //COMPUTERNETWORK_DATASTRUCTURE_H
