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
IDTransform IDTransTable[MAX_TABLE_SIZE];	//ID转换表
int IDcount = 0;					//转换表中的条目个数
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
    int iSend;

    //保存系统时间
    setTime();

    int find;
    u_short NewID;
    u_short* pID;

    //下面是服务器的具体操作
    while (1)
    {
        memset(recvBuf, 0, BUFSIZE); //将接收缓存先置为全0

        //接收DNS请求
        //函数：int recvfrom(int s, void* buf, int len, unsigned int flags, struct sockaddr* from, int* fromlen);
        //函数说明：recv()用来接收远程主机经指定的socket 传来的数据, 并把数据存到由参数buf 指向的内存空间, 参数len 为可接收数据的最大长度.
        //参数flags 一般设0, 其他数值定义请参考recv().参数from 用来指定欲传送的网络地址, 结构sockaddr 请参考bind().参数fromlen 为sockaddr 的结构长度.
        int recvnum = recvfrom(localSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientName, &clientLength);
        //错误反馈
        if (recvnum == SOCKET_ERROR || recvnum == 0)
            continue; //强制开始下一次循环
        else
        {
            char* domainName = (char*) malloc(FULL_DOMAIN_LENGTH);
            getDomainName(recvBuf, domainName);				//获取域名
            find = searchInLocalDNSTable(domainName, recordNum);		//在域名解析表中查找

            //在域名解析表中没有找到
            if (find == NOTFOUND)
            {
                //ID转换
                pID = (u_short*)malloc(sizeof(u_short*));
                memcpy(pID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                NewID = htons(ReplaceNewID(ntohs(*pID), clientName, FALSE));
                memcpy(recvBuf, &NewID, sizeof(u_short));

                //打印 时间 newID 功能 域名 IP
                PrintInfo(ntohs(NewID), find, domainName);

                //把recvbuf转发至指定的外部DNS服务器
                iSend = sendto(servSock, recvBuf, recvnum, 0, (SOCKADDR*)&serverName, sizeof(serverName));
                if (iSend == SOCKET_ERROR)
                {
                    //printf("sendto Failed: %s\n", strerror(WSAGetLastError()));
                    continue;
                }
                else if (iSend == 0)
                    continue;

                //delete pID; //释放动态分配的内存
                free(pID);

                clock_t start, stop; //定时
                double duration = 0;

                //接收来自外部DNS服务器的响应报文
                start = clock();
                recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientName, &clientLength);
                while ((recvnum == 0) || (recvnum == SOCKET_ERROR))
                {
                    recvnum = recvfrom(servSock, recvBuf, sizeof(recvBuf), 0, (SOCKADDR*)&clientName, &clientLength);
                    stop = clock();
                    duration = (double)(stop - start) / CLK_TCK;
                    if (duration > 5)
                    {
                        printf("Long Time No Response From Server.\n");
                        goto ps;
                    }
                }
                //ID转换
                pID = (u_short*)malloc(sizeof(u_short*));
                memcpy(pID, recvBuf, sizeof(u_short)); //报文前两字节为ID
                int GetId = ntohs(*pID); //ntohs的功能：将网络字节序转换为主机字节序
                u_short oID = htons(IDTransTable[GetId].oldID);
                memcpy(recvBuf, &oID, sizeof(u_short));
                IDTransTable[GetId].done = TRUE;

                //char* urlname;
                //memcpy(urlname, &(recvBuf[sizeof(DNSHDR)]), recvnum - 12);	//获取请求报文中的域名表示，要去掉DNS报文首部的12字节
                //char* NewIP;

                //打印 时间 newID 功能 域名 IP
                PrintInfo(ntohs(NewID), find, domainName);

                //从ID转换表中获取发出DNS请求者的信息
                clientName = IDTransTable[GetId].client;

                //printf("We get a answer from SERVER, now we give it back to client.\n");

                //把recvbuf转发至请求者处
                iSend = sendto(localSock, recvBuf, recvnum, 0, (SOCKADDR*)&clientName, sizeof(clientName));
                if (iSend == SOCKET_ERROR)
                {
                    //printf("sendto Failed: %s\n\n", strerror(WSAGetLastError()));
                    continue;
                }
                else if (iSend == 0)
                    continue;

                free(pID); //释放动态分配的内存
            }

            //在域名解析表中找到
            else
            {
                response(recvBuf, recvnum, find, localSock, clientName, domainName);
            }
        }
        ps:;
    }

    closesocket(servSock);
    closesocket(localSock);
    WSACleanup();				//释放ws2_32.dll动态链接库初始化时分配的资源

    system("pause");
    return 0;
}
