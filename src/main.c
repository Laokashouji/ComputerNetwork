//#define _CRT_SECURE_NO_WARNINGS
//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <windows.h>
#include <time.h>
#include "datastructure.h"
#include "MyFunction.h"
#include "MySocket.h"
#pragma comment(lib, "ws2_32.lib")

Translate DNSTable[MAX_TABLE_SIZE];		//DNS域名解析表
IDTable IDTransTable[MAX_TABLE_SIZE];	//ID转换表
int IDNum = 0;					//转换表中的条目个数
SYSTEMTIME TimeOfSys;                     //系统时间
int Day, Hour, Minute, Second, Milliseconds;//保存系统时间的变量


int main()
{

    //参考：https://wenku.baidu.com/view/ed7d64c852d380eb62946df4.html

    //初始化 DLL
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    //创建套接字
    SOCKET servSock = createSock(1);
    SOCKET localSock = createSock(1);

    //绑定套接字
    SOCKADDR_IN serverName, clientName, localName;	//本地DNS、外部DNS和请求端三个网络套接字地址
    setSockAddr(&localName, PORT, LOCAL_DNS_ADDRESS);
    setSockAddr(&serverName, PORT, DEF_DNS_ADDRESS);

    bindSock(localSock, &localName);

    char recvBuf[BUFSIZE]; //接收缓存
    int recordNum = loadLocalDNSTable(); //本地DNS文件有效行数
    int clientLength = sizeof(clientName);
    //保存系统时间
    setTime();
    //下面是服务器的具体操作
    while (1)
    {
        memset(recvBuf, 0, BUFSIZE); //将接收缓存先置为全0

        //接收DNS请求
        //函数：int recvfrom(int s, void* buf, int len, unsigned int flags, struct sockaddr* from, int* fromlen);
        //函数说明：recv()用来接收远程主机经指定的socket 传来的数据, 并把数据存到由参数buf 指向的内存空间, 参数len 为可接收数据的最大长度.
        //参数flags 一般设0, 其他数值定义请参考recv().参数from 用来指定欲传送的网络地址, 结构sockaddr 请参考bind().参数fromlen 为sockaddr 的结构长度.
        int recvnum = recvfrom(localSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientName, &clientLength);
        if (recvnum == SOCKET_ERROR || recvnum == 0)
            continue;
        else
        {
            char* domainName = (char*) malloc(FULL_DOMAIN_LENGTH + 1);
            getDomainName(recvBuf, domainName);				//获取域名
            int find = searchInLocalDNSTable(domainName, recordNum);		//在域名解析表中查找
            //打印请求信息
            PrintInfo(find, domainName);
            //在域名解析表中没有找到
            if (find == NOTFOUND)
            {
                //ID转换,用于实现多用户查询
                u_short* ID = (u_short*)malloc(sizeof(u_short*));
                memcpy(ID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                u_short NewID = htons(getNewID(ntohs(*ID), clientName));
                memcpy(recvBuf, &NewID, sizeof(u_short));

                //转发
                int sendLength = sendto(servSock, recvBuf, recvnum, 0, (SOCKADDR*)&serverName, sizeof(serverName));
                if (sendLength == SOCKET_ERROR || sendLength == 0)
                    continue;

                //接收来自外部DNS服务器的响应报文
                clock_t start = clock();
                double duration = 0;
                recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientName, &clientLength);
                while ((recvnum == 0) || (recvnum == SOCKET_ERROR))
                {
                    recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientName, &clientLength);
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
                sendto(localSock, recvBuf, recvnum, 0, (SOCKADDR*)&clientName, sizeof(clientName));
                free(ID); //释放动态分配的内存
            }

            //在域名解析表中找到
            else
            {
                //构造响应报文标志位
                char sendBuf[MAX_DNS_SIZE];
                memcpy(sendBuf, recvBuf, recvnum);
                u_short flags, ARRs;
                //构造相应报文应答资源个数
                if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0) {
                    flags = htons(0x8183);
                    ARRs = htons(0x0000);
                }
                else {
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
                u_long IP = (u_long)inet_addr(DNSTable[find].IP); //inet_addr为IP地址转化函数
                memcpy(answer + len, &IP, sizeof(u_long));
                len += sizeof(u_long);

                int queryLength = getQueryLength(sendBuf);

                //请求报文和响应部分共同组成DNS响应报文存入sendbuf
                //memcpy(sendBuf + queryLength, answer, len);
                memcpy(sendBuf + recvnum, answer, len);
                sendto(localSock, sendBuf, len + recvnum, 0, (SOCKADDR*)&clientName, sizeof(clientName));
            }
        }
    }

    closesocket(servSock);
    closesocket(localSock);
    WSACleanup();

    return 0;
}
