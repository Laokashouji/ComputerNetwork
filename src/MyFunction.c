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


#pragma  comment(lib, "Ws2_32.lib") //���� ws2_32.dll

extern Translate DNSTable[MAX_TABLE_SIZE];		//DNS����������
extern IDTable IDTransTable[MAX_TABLE_SIZE];	//IDת����
extern int IDNum;					//ת�����е���Ŀ����
extern SYSTEMTIME TimeOfSys;                     //ϵͳʱ��
extern int Day, Hour, Minute, Second, Milliseconds;//����ϵͳʱ��ı���

//���ر���txt�ļ�
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

//��ȡϵͳʱ��
void setTime() {
    GetLocalTime(&TimeOfSys);
    Day = TimeOfSys.wDay;
    Hour = TimeOfSys.wHour;
    Minute = TimeOfSys.wMinute;
    Milliseconds = TimeOfSys.wMilliseconds;
}

//��ȡDNS�����е�����
void getDomainName(char *recvbuf, char *domainName)
{
    char queryName[FULL_DOMAIN_LENGTH + 10];
    memset(domainName, 0, FULL_DOMAIN_LENGTH);
    memcpy(queryName, recvbuf + DNSHEADER, FULL_DOMAIN_LENGTH + 10);
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

//�ж��ܲ����ڱ����ҵ�DNS�����е��������ҵ������±�
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

//������IDת��Ϊ�µ�ID��������Ϣд��IDת������
u_short getNewID(u_short OldID, SOCKADDR_IN sock)
{
    while(IDTransTable[IDNum].unavailable == TRUE)
        IDNum = (IDNum + 1) % MAX_TABLE_SIZE;
    IDTransTable[IDNum].oldID = OldID;
    IDTransTable[IDNum].client = sock;
    IDTransTable[IDNum].unavailable = TRUE;

    return (u_short)IDNum;	//�Ա����±���Ϊ�µ�ID
}

//��ӡ ʱ�� ID ���� ���� IP
void PrintInfo(int find, char *domainName)
{
    //��ӡʱ��
    char timestr[128]="";
    time_t now;
    time(&now);
    strftime(timestr, 128, "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("%s\t", timestr);

    if (find == NOTFOUND)
    {
        //�м̹���
        printf("relay\t");
        //��ӡ����
        printf("%s\t", domainName);
        printf("\n");
    }
    else
    {
        if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0)  //������վ����
        {
            //���ι���
            printf("shield\t");
            printf("***%s\t", domainName);
            //��ӡIP
            printf("%s\n", DNSTable[find].IP);
        }
        else
        {
            //����������
            printf("LocalServer\t");
            //��ӡ����
            printf("***%s\t", domainName);
            //��ӡIP
            printf("%s\n", DNSTable[find].IP);
        }
    }
}

int getAnswer(char *answer)
 {
    int len = 0;
    //����,ָ����ʽ
     u_short name = htons(0xc00c);
     memcpy(answer, &name, sizeof(u_short));
     len += sizeof(u_short);
    //����
     u_short TypeA = htons(0x0001);
     memcpy(answer + len, &TypeA, sizeof(u_short));
     len += sizeof(u_short);
    //��
     u_short ClassA = htons(0x0001);
     memcpy(answer + len, &ClassA, sizeof(u_short));
     len += sizeof(u_short);
     //����ʱ��
     u_long timeLive = htonl(0x1e);
     memcpy(answer + len, &timeLive, sizeof(u_long));
     len += sizeof(u_long);
     //��Դ����,IPv4��ַ4�ֽ�
     u_short RDLength = htons(0x0004);
     memcpy(answer + len, &RDLength, sizeof(u_short));
     len += sizeof(u_short);

     return len;
 }

 int getQueryLength(char* buf)
 {
    //�ײ�12�ֽ�
    int i = 12;
    //��ѯ��
    while(buf[i] != 0)
        i++;
    i++;
    //��ѯ���ͺͲ�ѯ��
    return i + 4;
 }