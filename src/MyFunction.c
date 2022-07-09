//
// Created by LaoKaShouJi on 2022/7/7.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "datastructure.h"
#include "MyFunction.h"


#pragma  comment(lib, "Ws2_32.lib") //加载 ws2_32.dll

extern Translate DNSTable[MAX_TABLE_SIZE];		//DNS域名解析表
extern IDTransform IDTransTable[MAX_TABLE_SIZE];	//ID转换表
extern int IDcount;					//转换表中的条目个数
extern SYSTEMTIME TimeOfSys;                     //系统时间
extern int Day, Hour, Minute, Second, Milliseconds;//保存系统时间的变量

//加载本地txt文件
int loadLocalDNSTable()
{
    int i = 0, j = 0;
    char buf[MAX_TABLE_SIZE][FULL_DOMAIN_LENGTH];
    FILE* fp = fopen(LOCAL_DNS_FILE, "rb");
    if (!fp)
    {
        printf("Open file failed.\n");
        exit(-1);
    }
    for (i = 0; i < MAX_TABLE_SIZE; i++)
        if (fgets(buf[i], FULL_DOMAIN_LENGTH + MAX_IP4STRING_LENGTH, fp) == NULL)
            break;
    if (i == MAX_TABLE_SIZE - 1)
        printf("The local DNS-IP records are too much.\n");

    int num = 0;
    for (j = 0; j < i; j++)
    {
        char* ip = strtok(buf[j], " ");
        char* domain = strtok(NULL, " ");
        if (ip == NULL || domain == NULL)
            printf("can't load data \"%s %s\" in line %d\n", ip, domain, j);
        else
        {
            DNSTable[j].IP = ip;
            DNSTable[j].domain = domain;
            num++;
        }
    }
    fclose(fp);
    printf("Successfully read %d rows of local data\n", num);
    return num;
}

//获取系统时间
void setTime() {
    GetLocalTime(&TimeOfSys);
    Day = TimeOfSys.wDay;
    Hour = TimeOfSys.wHour;
    Minute = TimeOfSys.wMinute;
    Milliseconds = TimeOfSys.wMilliseconds;
}

//获取DNS请求中的域名
void getDomainName(char *recvbuf, char *domainName)
{
    char queryName[FULL_DOMAIN_LENGTH];
    memset(domainName, 0, FULL_DOMAIN_LENGTH);
    memcpy(queryName, &recvbuf[DNSHEADER], FULL_DOMAIN_LENGTH);
    int len = strlen(queryName);

    int i = 0, j = 0, k = 0;
    for (i = 0; i < len;)
    {
        if (queryName[i] > 0 && queryName[i] <= 63)
            for (j = queryName[i], i++; j > 0; j--, i++, k++)
                domainName[k] = queryName[i];
        if (queryName[i] != 0)
            domainName[k++] = '.';
        else break;
    }
    domainName[k] = '\0';
}

//判断能不能在本中找到DNS请求中的域名，找到返回下标
int searchInLocalDNSTable(char* domainName, int num)
{
    char *domain = (char*)malloc(sizeof(char) * (FULL_DOMAIN_LENGTH + 2));
    strcpy(domain, domainName);
    strcat(domain, "\r\n");

    for (int i = 0; i < num+2; i++)
        if (DNSTable[i].domain)
            if (strcmp(DNSTable[i].domain, domain) == 0)
                return i;
    return NOTFOUND;
}

//将请求ID转换为新的ID，并将信息写入ID转换表中
u_short ReplaceNewID(u_short OldID, SOCKADDR_IN temp, BOOL ifdone)
{
    srand(time(NULL)); //随机数种子
    IDTransTable[IDcount].oldID = OldID;
    IDTransTable[IDcount].client = temp;
    IDTransTable[IDcount].done = ifdone;
    IDcount++; //ID转换表数目要更新~

    return (u_short)(IDcount - 1);	//以表中下标作为新的ID
}

//打印 时间 ID 功能 域名 IP
void PrintInfo(u_short ID, int find, char* domainName)
{
    //打印时间
    GetLocalTime(&TimeOfSys);
    //输出指定长度的字符串, 超长时不截断, 不足时左对齐:
    //printf("%-ns", str);            --n 为指定长度的10进制数值
    int Btime;
    int Ltime;
    Btime = ((((TimeOfSys.wDay - Day) * 24 + TimeOfSys.wHour - Hour) * 60 + TimeOfSys.wMinute - Minute) * 60) + TimeOfSys.wSecond - Second;
    Ltime = abs(TimeOfSys.wMilliseconds - Milliseconds);
    printf("%d.%d   %d", Btime, Ltime, ID);
    printf("    ");

    //在表中没有找到DNS请求中的域名
    if (find == NOTFOUND)
    {
        //中继功能
        printf("中继");
        printf("    ");
        //打印域名
        printf("%s", domainName);
        printf("    ");
        //打印IP
        printf("\n");
    }

        //在表中找到DNS请求中的域名
    else if(find == ISFOUND)
    {
        if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0)  //不良网站拦截
        {
            //屏蔽功能
            printf("屏蔽");
            printf("    ");
            //打印域名(加*)
            //打印域名
            printf("***%s", domainName);
            printf("    ");
            //打印IP
            printf("%s\n", DNSTable[find].IP);
        }

            //检索结果为普通IP地址，则向客户返回这个地址
        else
        {
            //服务器功能
            printf("Local服务器");
            printf("    ");
            //打印域名
            printf("***%s", domainName);
            printf("    ");
            //打印IP
            printf("%s\n", DNSTable[find].IP);
        }
    }
}

void response(char *recvBuf, int recvnum, int find, SOCKET localSock, SOCKADDR_IN clientName, char *domainName) {
    //获取请求报文的ID
    u_short* ID = (u_short*)malloc(sizeof(u_short*));
    memcpy(ID, recvBuf, sizeof(u_short));

    //打印 时间 newID 功能 域名 IP
    PrintInfo(ID, ISFOUND, domainName);

    //构造响应报文头
    char sendBuf[BUFSIZ];
    memcpy(sendBuf, recvBuf, recvnum); //拷贝请求报文
    u_short AFlag = htons(0x8180); //htons的功能：将主机字节序转换为网络字节序，即大端模式(big-endian) 0x8180为DNS响应报文的标志Flags字段
    memcpy(&sendBuf[2], &AFlag, sizeof(u_short)); //修改标志域,绕开ID的两字节

    //修改回答数域
    if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0)
        AFlag = htons(0x0000);	//屏蔽功能：回答数为0
    else
        AFlag = htons(0x0001);	//服务器功能：回答数为1
    memcpy(&sendBuf[6], &AFlag, sizeof(u_short)); //修改回答记录数，绕开ID两字节、Flags两字节、问题记录数两字节

    int curLen = 0; //不断更新的长度

    //构造DNS响应部分
    //参考：http://c.biancheng.net/view/6457.html
    char answer[16];
    u_short Name = htons(0xc00c);
    memcpy(answer, &Name, sizeof(u_short));
    curLen += sizeof(u_short);

    u_short TypeA = htons(0x0001);
    memcpy(answer + curLen, &TypeA, sizeof(u_short));
    curLen += sizeof(u_short);

    u_short ClassA = htons(0x0001);
    memcpy(answer + curLen, &ClassA, sizeof(u_short));
    curLen += sizeof(u_short);

    //TTL四字节
    unsigned long timeLive = htonl(0x7b);
    memcpy(answer + curLen, &timeLive, sizeof(unsigned long));
    curLen += sizeof(unsigned long);

    u_short RDLength = htons(0x0004);
    memcpy(answer + curLen, &RDLength, sizeof(u_short));
    curLen += sizeof(u_short);

    unsigned long IP = (unsigned long)inet_addr(DNSTable[find].IP); //inet_addr为IP地址转化函数
    memcpy(answer + curLen, &IP, sizeof(unsigned long));
    curLen += sizeof(unsigned long);
    curLen += recvnum;

    //请求报文和响应部分共同组成DNS响应报文存入sendbuf
    memcpy(sendBuf + recvnum, answer, curLen);
    sendto(localSock, sendBuf, curLen, 0, (SOCKADDR*)&clientName, sizeof(clientName));

    free(ID); //释放动态分配的内存
}


