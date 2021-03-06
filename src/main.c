
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
#include <stdlib.h>
#include <unistd.h>
#include "datastructure.h"
#include "MyFunction.h"
#include "MySocket.h"

DNSTable DNSTransTable[MAX_TABLE_SIZE + MAX_CACHE_LENGTH];
IDTable IDTransTable[MAX_TABLE_SIZE];
u_short IDNum = 0;
int CacheNum = 0;

int main() {

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    //创建套接字
    SOCKET servSock = createSock(1);
    SOCKET localSock = createSock(1);

    struct sockaddr_in serverName, clientName, localName;
    setSockAddr(&localName, PORT, LOCAL_DNS_ADDRESS);
    setSockAddr(&serverName, PORT, DEF_DNS_ADDRESS);

    bindSock(&localSock, &localName);

    //读取本地DNS文件
    int recordNum = loadLocalDNSTable();

    int clientLength = sizeof(clientName);
    char recvBuf[BUFSIZE];

    while (1) {
        memset(recvBuf, 0, BUFSIZE);

        int recvnum = recvfrom(localSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR *) &clientName, &clientLength);
        if (recvnum == SOCKET_ERROR || recvnum == 0) {
            usleep(50);
            continue;
        }
        else {
            char *domainName = (char *) malloc(FULL_DOMAIN_LENGTH + 1);
            u_short type = getDomainName(recvBuf, domainName);
            int find = searchInLocalDNSTable(domainName, recordNum);
            //打印请求信息
            PrintInfo(find, domainName);
            //在域名解析表中没有找到
            if (find == NOTFOUND) {

                u_short *ID = (u_short *) malloc(sizeof(u_short *));
                memcpy(ID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                //获取新ID
                u_short NewID = htons(getNewID(ntohs(*ID), clientName));
                memcpy(recvBuf, &NewID, sizeof(u_short));
                NewID = ntohs(NewID);
                //转发
                int sendLength = sendto(servSock, recvBuf, recvnum, 0, (SOCKADDR *) &serverName, sizeof(serverName));
                if (sendLength == SOCKET_ERROR || sendLength == 0) {
                    /*if (sendLength == SOCKET_ERROR) {
                        int test = WSAGetLastError();
                        printf("%d", test);
                    }*/
                    IDTransTable[NewID].unavailable = FALSE;
                    continue;
                }
                IDTransTable[NewID].len = sendLength;

                //接收来自外部DNS服务器的响应报文, 重复接收保证可以收到可能有的以前未收到的应答
                recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR *) &clientName, &clientLength);
                while (!((recvnum == 0) || (recvnum == SOCKET_ERROR))) {
                    //ID转换
                    memcpy(ID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                    u_short newID = ntohs(*ID); //ntohs的功能：将网络字节序转换为主机字节序
                    u_short oldID = htons(IDTransTable[newID].oldID);
                    memcpy(recvBuf, &oldID, sizeof(u_short));

                    //从ID转换表中获取发出DNS请求者的信息
                    clientName = IDTransTable[newID].client;
                    if(IDTransTable[newID].unavailable == FALSE || recvnum < IDTransTable[newID].len + 16)
                        break;
                    IDTransTable[newID].unavailable = FALSE;
                    //把recvbuf转发至请求者处
                    sendto(localSock, recvBuf, recvnum, 0, (SOCKADDR *) &clientName, sizeof(clientName));

                    //获取IP添加到Cache中
                    getDomainName(recvBuf, domainName);
                    char *IP = (char *) malloc(MAX_IP4STRING_LENGTH + 1);
                    getIP(recvBuf, IDTransTable[newID].len, IP);
                    if(IP != NULL && IP != "")
                        addCache(domainName, IP);
                    free(IP);

                    recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR *) &clientName, &clientLength);
                }


                free(ID);

            }
            //在本地表和Cahce中找到
            else {

                DNSTransTable[find].time = 0;

                //构造响应报文标志位
                char sendBuf[MAX_DNS_SIZE];
                memcpy(sendBuf, recvBuf, recvnum);
                u_short flags, ARRs;

                //构造相应报文应答资源个数
                if (strcmp(DNSTransTable[find].IP, "0.0.0.0") == 0 || type != 1) {
                    flags = htons(0x8183);
                    ARRs = htons(0x0000);
                } else {
                    flags = htons(0x8180);
                    ARRs = htons(0x0001);
                }
                memcpy(sendBuf + 2, &flags, sizeof(u_short));
                memcpy(sendBuf + 6, &ARRs, sizeof(u_short));

                char answer[16];
                int len = getAnswer(answer);

                u_long IP = (u_long) inet_addr(DNSTransTable[find].IP);
                memcpy(answer + len, &IP, sizeof(u_long));
                len += sizeof(u_long);

                memcpy(sendBuf + recvnum, answer, len);
                sendto(localSock, sendBuf, len + recvnum, 0, (SOCKADDR *) &clientName, sizeof(clientName));
            }
        }
    }
#ifdef _WIN32
    closesocket(servSock);
    closesocket(localSock);
    WSACleanup();
#else
		close(servSock);
		close(localSock);
#endif

    return 0;
}
