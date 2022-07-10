//
// Created by LaoKaShouJi on 2022/7/7.
//
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include<winSock2.h>
#include<windows.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include<arpa/inet.h>//selcet
#include<unistd.h>//uni std
#include<string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ws2tcpip.h>
#include "datastructure.h"
#include "MyFunction.h"

extern DNSTable DNSTransTable[MAX_TABLE_SIZE + MAX_CACHE_LENGTH];        //DNS域名解析表
extern IDTable IDTransTable[MAX_TABLE_SIZE];    //ID转换表
extern int IDNum;                    //转换表中的条目个数
extern int CacheNum;

//加载本地txt文件
int loadLocalDNSTable() {
    int i = 0, j = 0;
    char buf[MAX_TABLE_SIZE][FULL_DOMAIN_LENGTH];
    FILE *fp = fopen(LOCAL_DNS_FILE, "rb");
    if (!fp) {
        printf("Open file failed.\n");
        exit(-1);
    }
    for (i = 0; i < MAX_TABLE_SIZE; i++)
        if (fgets(buf[i], FULL_DOMAIN_LENGTH + MAX_IP4STRING_LENGTH, fp) == NULL)
            break;
    if (i == MAX_TABLE_SIZE - 1)
        printf("The local DNS-IP records are too much.\n");

    int num = 0;
    for (j = 0; j < i; j++) {
        char *ip = strtok(buf[j], " ");
        char *domain = strtok(NULL, " ");
        if (ip == NULL || domain == NULL)
            printf("can't load data \"%s %s\" in line %d\n", ip, domain, j);
        else {
            DNSTransTable[j].IP = ip;
            DNSTransTable[j].domain = domain;
            num++;
        }
    }
    fclose(fp);
    printf("Successfully read %d rows of local data\n", num);
    return num;
}

//获取DNS请求中的域名
u_short getDomainName(char *recvbuf, char *domainName) {
    char queryName[FULL_DOMAIN_LENGTH + 10];
    memset(domainName, 0, FULL_DOMAIN_LENGTH);
    memcpy(queryName, recvbuf + DNSHEADER, FULL_DOMAIN_LENGTH + 10);
    int len = strlen(queryName);

    int i = 0, j = 0, k = 0;
    for (i = 0; i < len;) {
        if (queryName[i] > 0 && queryName[i] <= 63)
            for (j = queryName[i], i++; j > 0; j--, i++, k++)
                domainName[k] = queryName[i];
        if (queryName[i] != 0)
            domainName[k++] = '.';
        else break;
    }
    domainName[k] = '\0';

    u_short *buf = (u_short *) malloc(sizeof(u_short *));
    memcpy(buf, recvbuf + DNSHEADER + k + 2, sizeof(u_short));
    u_short type = ntohs(*buf);
    return type;
}

void getIP(char *recvBuf, int sendLen, char *IP) {
    u_short *buf = (u_short *) malloc(sizeof(u_short *));
    memcpy(buf, recvBuf + 6, sizeof(u_short));
    int ARRs = ntohs(*buf);
    int j = 0;
    for (int i = 0; i < ARRs; i++) {
        memcpy(buf, recvBuf + sendLen + j + 2, sizeof(u_short));
        u_short type = ntohs(*buf);
        if (type == 1) {
            char IPbuf[MAX_IP4STRING_LENGTH + 1];
            inet_ntop(AF_INET, recvBuf + sendLen + j + 12, IPbuf, sizeof(IPbuf));
            strcpy(IP, IPbuf);
            return;
        } else {
            memcpy(buf, recvBuf + sendLen + j + 10, sizeof(u_short)); //报文前两字节为ID
            u_short DL = ntohs(*buf);
            j += 12 + DL;
            continue;
        }
    }
    IP = NULL;
}

//判断能不能在本中找到DNS请求中的域名，找到返回下标
int searchInLocalDNSTable(char *domainName, int num) {
    char *domain = (char *) malloc(sizeof(char) * (FULL_DOMAIN_LENGTH + 2));
    strcpy(domain, domainName);
    strcat(domain, "\r\n");

    for (int i = 0; i < num; i++)
        if (DNSTransTable[i].domain)
            if (strcmp(DNSTransTable[i].domain, domain) == 0)
                return i;

    for (int i = 0; i < CacheNum; i++) {
        if (DNSTransTable[i + MAX_TABLE_SIZE].domain)
            if (strcmp(DNSTransTable[i + MAX_TABLE_SIZE].domain, domain) == 0)
                return i + MAX_TABLE_SIZE;
    }

    return NOTFOUND;
}

//将请求ID转换为新的ID，并将信息写入ID转换表中
u_short getNewID(u_short OldID, SOCKADDR_IN sock) {
    while (IDTransTable[IDNum].unavailable == TRUE)
        IDNum = (IDNum + 1) % MAX_TABLE_SIZE;
    IDTransTable[IDNum].oldID = OldID;
    IDTransTable[IDNum].client = sock;
    IDTransTable[IDNum].unavailable = TRUE;

    return (u_short) IDNum;    //以表中下标作为新的ID
}

//打印 时间 ID 功能 域名 IP
void PrintInfo(int find, char *domainName) {
    //打印时间
    char timestr[128] = "";
    time_t now;
    time(&now);
    strftime(timestr, 128, "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("%s\t", timestr);

    if (find == NOTFOUND) {
        //中继功能
        printf("relay\t");
        //打印域名
        printf("%s\t", domainName);
        printf("\n");
    } else {
        if (strcmp(DNSTransTable[find].IP, "0.0.0.0") == 0)  //不良网站拦截
        {
            //屏蔽功能
            printf("shield\t");
            printf("***%s\t", domainName);
            //打印IP
            printf("%s\n", DNSTransTable[find].IP);
        } else {
            //服务器功能
            printf("LocalServer\t");
            //打印域名
            printf("***%s\t", domainName);
            //打印IP
            printf("%s\n", DNSTransTable[find].IP);
        }
    }
}

int getAnswer(char *answer) {
    int len = 0;
    //域名,指针形式
    u_short name = htons(0xc00c);
    memcpy(answer, &name, sizeof(u_short));
    len += sizeof(u_short);
    //类型
    u_short TypeA = htons(0x0001);
    memcpy(answer + len, &TypeA, sizeof(u_short));
    len += sizeof(u_short);
    //类
    u_short ClassA = htons(0x0001);
    memcpy(answer + len, &ClassA, sizeof(u_short));
    len += sizeof(u_short);
    //生存时间
    u_long timeLive = htonl(0x1e);
    memcpy(answer + len, &timeLive, sizeof(u_long));
    len += sizeof(u_long);
    //资源长度,IPv4地址4字节
    u_short RDLength = htons(0x0004);
    memcpy(answer + len, &RDLength, sizeof(u_short));
    len += sizeof(u_short);

    return len;
}

void addCache(char *domain, char *IP) {
    char *domainName = (char *) malloc(sizeof(char) * (FULL_DOMAIN_LENGTH + 2));
    char *IPName = (char *) malloc(sizeof(char) * (MAX_IP4STRING_LENGTH + 1));
    strcpy(domainName, domain);
    strcpy(IPName, IP);
    strcat(domainName, "\r\n");

    if (CacheNum < MAX_CACHE_LENGTH) {
        DNSTransTable[MAX_TABLE_SIZE + CacheNum].IP = IPName;
        DNSTransTable[MAX_TABLE_SIZE + CacheNum].domain = domainName;
        DNSTransTable[MAX_TABLE_SIZE + CacheNum].time = 0;
        CacheNum++;
    } else {
        int maxj = -1, maxt = 0;
        for (int i = 0; i < MAX_CACHE_LENGTH; i++) {
            if (DNSTransTable[MAX_TABLE_SIZE + i].time > maxt) {
                maxt = DNSTransTable[MAX_TABLE_SIZE + i].time;
                maxj = i;
            }
            DNSTransTable[MAX_TABLE_SIZE + i].time++;
        }
        DNSTransTable[MAX_TABLE_SIZE + maxj].IP = IPName;
        DNSTransTable[MAX_TABLE_SIZE + maxj].domain = domainName;
        DNSTransTable[MAX_TABLE_SIZE + maxj].time = 0;
    }
}
