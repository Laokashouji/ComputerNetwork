//
// Created by LaoKaShouJi on 2022/7/6.
//

#ifndef C_MYFUNCTION_H
#define C_MYFUNCTION_H
#pragma once

int loadLocalDNSTable(); //加载本地txt文件
u_short getDomainName(char *recvbuf, char *domainName); //获取DNS请求中的域名
void getIP(char *recvBuf, int sendLen, char *IP);
int searchInLocalDNSTable(char* domainName, int num);//判断能不能在本中找到DNS请求中的域名，找到返回下标
u_short getNewID(u_short OldID, SOCKADDR_IN sock); //将请求ID转换为新的ID，并将信息写入ID转换表中
void PrintInfo(int find, char *domainName); //打印 时间 ID 功能 域名 IP
int getAnswer(char* answer);
void addCache(char *domain, char *IP);
#endif //C_MYFUNCTION_H
