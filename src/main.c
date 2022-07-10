
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include<winSock2.h>
#include<windows.h>
#pragma comment(lib, "ws2_32.lib")//链接此动态链接库 windows特有
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
#include <time.h>
#include <unistd.h>
#include "datastructure.h"
#include "MyFunction.h"
#include "MySocket.h"

DNSTable DNSTransTable[MAX_TABLE_SIZE + MAX_CACHE_LENGTH];        //DNS域名解析表
IDTable IDTransTable[MAX_TABLE_SIZE];    //ID转换表
int IDNum = 0;                    //转换表中的条目个数
int CacheNum = 0;

int main() {

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    //创建套接字
    SOCKET servSock = createSock(1);
    SOCKET localSock = createSock(1);

    //绑定套接字
    SOCKADDR_IN serverName, clientName, localName;    //本地DNS、外部DNS和请求端三个网络套接字地址
    setSockAddr(&localName, PORT, LOCAL_DNS_ADDRESS);
    setSockAddr(&serverName, PORT, DEF_DNS_ADDRESS);

    bindSock(localSock, &localName);

    int recordNum = loadLocalDNSTable(); //本地DNS文件有效行数

    int clientLength = sizeof(clientName);
    char recvBuf[BUFSIZE]; //接收缓存
    //下面是服务器的具体操作
    while (1) {
        memset(recvBuf, 0, BUFSIZE); //将接收缓存先置为全0

        int recvnum = recvfrom(localSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR *) &clientName, &clientLength);
        if (recvnum == SOCKET_ERROR || recvnum == 0)
            continue;
        else {
            char *domainName = (char *) malloc(FULL_DOMAIN_LENGTH + 1);
            u_short type = getDomainName(recvBuf, domainName);                //获取域名
            int find = searchInLocalDNSTable(domainName, recordNum);        //在域名解析表中查找
            //打印请求信息
            PrintInfo(find, domainName);
            //在域名解析表中没有找到
            if (find == NOTFOUND) {

                u_short *ID = (u_short *) malloc(sizeof(u_short *));
                memcpy(ID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                u_short NewID = htons(getNewID(ntohs(*ID), clientName));
                memcpy(recvBuf, &NewID, sizeof(u_short));

                //转发
                int sendLength = sendto(servSock, recvBuf, recvnum, 0, (SOCKADDR *) &serverName, sizeof(serverName));
                if (sendLength == SOCKET_ERROR || sendLength == 0)
                    continue;

                //接收来自外部DNS服务器的响应报文
                clock_t start = clock();
                double duration = 0;
                recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR *) &clientName, &clientLength);
                while ((recvnum == 0) || (recvnum == SOCKET_ERROR)) {
                    recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR *) &clientName, &clientLength);
                    usleep(100);
                    duration = (clock() - start) / CLK_TCK > WAIT_TIME;
                    if (duration > WAIT_TIME)
                        break;
                }
                if (duration > WAIT_TIME)
                    continue;

                //ID转换
                memcpy(ID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                int newID = ntohs(*ID); //ntohs的功能：将网络字节序转换为主机字节序
                u_short oldID = htons(IDTransTable[newID].oldID);
                memcpy(recvBuf, &oldID, sizeof(u_short));
                IDTransTable[newID].unavailable = FALSE;
                //从ID转换表中获取发出DNS请求者的信息
                clientName = IDTransTable[newID].client;

                //把recvbuf转发至请求者处
                sendto(localSock, recvBuf, recvnum, 0, (SOCKADDR *) &clientName, sizeof(clientName));

                char *IP = (char *) malloc(MAX_IP4STRING_LENGTH + 1);
                getIP(recvBuf, sendLength, IP);
                if(IP != NULL && IP != "")
                    addCache(domainName, IP);
                free(IP);
                free(ID); //释放动态分配的内存

            }
                //在域名解析表中找到
            else {
                /*if(type != 1)
                    continue;*/
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
                memcpy(sendBuf + 6, &ARRs, sizeof(u_short)); //修改回答记录数，绕开ID两字节、Flags两字节、问题记录数两字节

                //构造DNS应答部分
                //参考：http://c.biancheng.net/view/6457.html
                char answer[16];
                int len = getAnswer(answer);

                //存入本地数据中对应域名的IP地址
                u_long IP = (u_long) inet_addr(DNSTransTable[find].IP); //inet_addr为IP地址转化函数
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
