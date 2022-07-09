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
extern IDTransform IDTransTable[MAX_TABLE_SIZE];	//IDת����
extern int IDcount;					//ת�����е���Ŀ����
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
u_short ReplaceNewID(u_short OldID, SOCKADDR_IN temp, BOOL ifdone)
{
    srand(time(NULL)); //���������
    IDTransTable[IDcount].oldID = OldID;
    IDTransTable[IDcount].client = temp;
    IDTransTable[IDcount].done = ifdone;
    IDcount++; //IDת������ĿҪ����~

    return (u_short)(IDcount - 1);	//�Ա����±���Ϊ�µ�ID
}

//��ӡ ʱ�� ID ���� ���� IP
void PrintInfo(u_short ID, int find, char* domainName)
{
    //��ӡʱ��
    GetLocalTime(&TimeOfSys);
    //���ָ�����ȵ��ַ���, ����ʱ���ض�, ����ʱ�����:
    //printf("%-ns", str);            --n Ϊָ�����ȵ�10������ֵ
    int Btime;
    int Ltime;
    Btime = ((((TimeOfSys.wDay - Day) * 24 + TimeOfSys.wHour - Hour) * 60 + TimeOfSys.wMinute - Minute) * 60) + TimeOfSys.wSecond - Second;
    Ltime = abs(TimeOfSys.wMilliseconds - Milliseconds);
    printf("%d.%d   %d", Btime, Ltime, ID);
    printf("    ");

    //�ڱ���û���ҵ�DNS�����е�����
    if (find == NOTFOUND)
    {
        //�м̹���
        printf("�м�");
        printf("    ");
        //��ӡ����
        printf("%s", domainName);
        printf("    ");
        //��ӡIP
        printf("\n");
    }

        //�ڱ����ҵ�DNS�����е�����
    else if(find == ISFOUND)
    {
        if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0)  //������վ����
        {
            //���ι���
            printf("����");
            printf("    ");
            //��ӡ����(��*)
            //��ӡ����
            printf("***%s", domainName);
            printf("    ");
            //��ӡIP
            printf("%s\n", DNSTable[find].IP);
        }

            //�������Ϊ��ͨIP��ַ������ͻ����������ַ
        else
        {
            //����������
            printf("Local������");
            printf("    ");
            //��ӡ����
            printf("***%s", domainName);
            printf("    ");
            //��ӡIP
            printf("%s\n", DNSTable[find].IP);
        }
    }
}

void response(char *recvBuf, int recvnum, int find, SOCKET localSock, SOCKADDR_IN clientName, char *domainName) {
    //��ȡ�����ĵ�ID
    u_short* ID = (u_short*)malloc(sizeof(u_short*));
    memcpy(ID, recvBuf, sizeof(u_short));

    //��ӡ ʱ�� newID ���� ���� IP
    PrintInfo(ID, ISFOUND, domainName);

    //������Ӧ����ͷ
    char sendBuf[BUFSIZ];
    memcpy(sendBuf, recvBuf, recvnum); //����������
    u_short AFlag = htons(0x8180); //htons�Ĺ��ܣ��������ֽ���ת��Ϊ�����ֽ��򣬼����ģʽ(big-endian) 0x8180ΪDNS��Ӧ���ĵı�־Flags�ֶ�
    memcpy(&sendBuf[2], &AFlag, sizeof(u_short)); //�޸ı�־��,�ƿ�ID�����ֽ�

    //�޸Ļش�����
    if (strcmp(DNSTable[find].IP, "0.0.0.0") == 0)
        AFlag = htons(0x0000);	//���ι��ܣ��ش���Ϊ0
    else
        AFlag = htons(0x0001);	//���������ܣ��ش���Ϊ1
    memcpy(&sendBuf[6], &AFlag, sizeof(u_short)); //�޸Ļش��¼�����ƿ�ID���ֽڡ�Flags���ֽڡ������¼�����ֽ�

    int curLen = 0; //���ϸ��µĳ���

    //����DNS��Ӧ����
    //�ο���http://c.biancheng.net/view/6457.html
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

    //TTL���ֽ�
    unsigned long timeLive = htonl(0x7b);
    memcpy(answer + curLen, &timeLive, sizeof(unsigned long));
    curLen += sizeof(unsigned long);

    u_short RDLength = htons(0x0004);
    memcpy(answer + curLen, &RDLength, sizeof(u_short));
    curLen += sizeof(u_short);

    unsigned long IP = (unsigned long)inet_addr(DNSTable[find].IP); //inet_addrΪIP��ַת������
    memcpy(answer + curLen, &IP, sizeof(unsigned long));
    curLen += sizeof(unsigned long);
    curLen += recvnum;

    //�����ĺ���Ӧ���ֹ�ͬ���DNS��Ӧ���Ĵ���sendbuf
    memcpy(sendBuf + recvnum, answer, curLen);
    sendto(localSock, sendBuf, curLen, 0, (SOCKADDR*)&clientName, sizeof(clientName));

    free(ID); //�ͷŶ�̬������ڴ�
}
