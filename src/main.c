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

Translate DNSTable[AMOUNT];		//DNS域名解析表
IDTransform IDTransTable[AMOUNT];	//ID转换表
int IDcount = 0;					//转换表中的条目个数
char domainName[URL_LENGTH];					//域名
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

    char sendBuf[BUFSIZE]; //发送缓存
    char recvBuf[BUFSIZE]; //接收缓存
    int recordNum = InitialDNSTable(); //本地DNS文件有效行数

    int clientLength = sizeof(clientName);
    int iSend;

    //保存系统时间
    setTime();

    int find;
    unsigned short NewID;
    unsigned short* pID;

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
        {
            //printf("Recvfrom Failed: %s\n", strerror(WSAGetLastError()));
            continue; //强制开始下一次循环
        }
        else
        {
            GetUrl(recvBuf, recvnum);				//获取域名
            find = IsFind(domainName, recordNum);		//在域名解析表中查找

            //printf("We have get the url: %s\n", domainName);
            //printf("%d\n", find);

            //开始分情况讨论
            //在域名解析表中没有找到
            if (find == NOTFOUND)
            {
                //printf("We dont find this url, will get a new ID and forward to SERVER.\n");
                //ID转换
                //pID = new (unsigned short);
                pID = (unsigned short*)malloc(sizeof(unsigned short*));
                memcpy(pID, recvBuf, sizeof(unsigned short)); //报文前两字节为ID
                NewID = htons(ReplaceNewID(ntohs(*pID), clientName, FALSE));
                memcpy(recvBuf, &NewID, sizeof(unsigned short));

                //打印 时间 newID 功能 域名 IP
                PrintInfo(ntohs(NewID), find);

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
                pID = (unsigned short*)malloc(sizeof(unsigned short*));
                memcpy(pID, recvBuf, sizeof(unsigned short)); //报文前两字节为ID
                int GetId = ntohs(*pID); //ntohs的功能：将网络字节序转换为主机字节序
                unsigned short oID = htons(IDTransTable[GetId].oldID);
                memcpy(recvBuf, &oID, sizeof(unsigned short));
                IDTransTable[GetId].done = TRUE;

                //char* urlname;
                //memcpy(urlname, &(recvBuf[sizeof(DNSHDR)]), recvnum - 12);	//获取请求报文中的域名表示，要去掉DNS报文首部的12字节
                //char* NewIP;

                //打印 时间 newID 功能 域名 IP
                PrintInfo(ntohs(NewID), find);

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
                //printf("We have find this url.\n");
                //获取请求报文的ID
                pID = (unsigned short*)malloc(sizeof(unsigned short*));
                memcpy(pID, recvBuf, sizeof(unsigned short));

                //转换ID
                unsigned short nID = ReplaceNewID(ntohs(*pID), clientName, FALSE);

                //printf("We have get a new ID, now we will create an answer.\n");

                //打印 时间 newID 功能 域名 IP
                PrintInfo(nID, find);
                //参考：https://blog.csdn.net/weixin_34192993/article/details/87949701
                //构造响应报文头
                memcpy(sendBuf, recvBuf, recvnum); //拷贝请求报文
                unsigned short AFlag = htons(0x8180); //htons的功能：将主机字节序转换为网络字节序，即大端模式(big-endian) 0x8180为DNS响应报文的标志Flags字段
                memcpy(&sendBuf[2], &AFlag, sizeof(unsigned short)); //修改标志域,绕开ID的两字节

                //修改回答数域
                if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0)
                    AFlag = htons(0x0000);	//屏蔽功能：回答数为0
                else
                    AFlag = htons(0x0001);	//服务器功能：回答数为1
                memcpy(&sendBuf[6], &AFlag, sizeof(unsigned short)); //修改回答记录数，绕开ID两字节、Flags两字节、问题记录数两字节

                int curLen = 0; //不断更新的长度

                //构造DNS响应部分
                //参考：http://c.biancheng.net/view/6457.html
                char answer[16];
                unsigned short Name = htons(0xc00c);
                memcpy(answer, &Name, sizeof(unsigned short));
                curLen += sizeof(unsigned short);

                unsigned short TypeA = htons(0x0001);
                memcpy(answer + curLen, &TypeA, sizeof(unsigned short));
                curLen += sizeof(unsigned short);

                unsigned short ClassA = htons(0x0001);
                memcpy(answer + curLen, &ClassA, sizeof(unsigned short));
                curLen += sizeof(unsigned short);

                //TTL四字节
                unsigned long timeLive = htonl(0x7b);
                memcpy(answer + curLen, &timeLive, sizeof(unsigned long));
                curLen += sizeof(unsigned long);

                unsigned short RDLength = htons(0x0004);
                memcpy(answer + curLen, &RDLength, sizeof(unsigned short));
                curLen += sizeof(unsigned short);

                unsigned long IP = (unsigned long)inet_addr(DNSTable[find].IP); //inet_addr为IP地址转化函数
                memcpy(answer + curLen, &IP, sizeof(unsigned long));
                curLen += sizeof(unsigned long);
                curLen += recvnum;


                //请求报文和响应部分共同组成DNS响应报文存入sendbuf
                memcpy(sendBuf + recvnum, answer, curLen);

                //printf("Create Over, give it to client.\n");

                clock_t Nstart, Nstop; //clock_t为clock()函数返回的变量类型
                double Nduration;


                //发送DNS响应报文
                Nstart = clock();
                iSend = sendto(localSock, sendBuf, curLen, 0, (SOCKADDR*)&clientName, sizeof(clientName));
                //if (iSend == SOCKET_ERROR)
                //{
                //	//printf("sendto Failed: %s\n", strerror(WSAGetLastError()));
                //	Nstop = clock();
                //	Nduration = (double)(Nstop - Nstart) / CLK_TCK;
                //	if (Nduration > 1)
                //		goto ps;
                //	else
                //		continue;
                //}
                //else if (iSend == 0)
                //	break;

                free(pID); //释放动态分配的内存

                //printf("\nThis loop is over, thanks.\n\n");
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
