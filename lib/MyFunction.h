//
// Created by LaoKaShouJi on 2022/7/6.
//

#ifndef C_MYFUNCTION_H
#define C_MYFUNCTION_H
#pragma once

int loadLocalDNSTable(); //加载本地txt文件
void setTime();//获取系统时间
void getDomainName(char *recvbuf, char *domainName); //获取DNS请求中的域名
int searchInLocalDNSTable(char* domainName, int num);//判断能不能在本中找到DNS请求中的域名，找到返回下标
u_short ReplaceNewID(u_short OldID, SOCKADDR_IN temp, BOOL ifdone); //将请求ID转换为新的ID，并将信息写入ID转换表中
void response(char *recvBuf, int recvnum, int find, SOCKET localSock, SOCKADDR_IN clientName, char *domainName);
void PrintInfo(u_short ID, int find, char* domainName); //打印 时间 ID 功能 域名 IP

#endif //C_MYFUNCTION_H
