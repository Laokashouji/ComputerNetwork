//
// Created by LaoKaShouJi on 2022/7/6.
//

#ifndef C_MYFUNCTION_H
#define C_MYFUNCTION_H
#pragma once

int InitialDNSTable(char* path); //加载本地txt文件
void GetUrl(char* recvbuf, int recvnum); //获取DNS请求中的域名
int IsFind(char* url, int num);//判断能不能在本中找到DNS请求中的域名，找到返回下标
unsigned short ReplaceNewID(unsigned short OldID, SOCKADDR_IN temp, BOOL ifdone); //将请求ID转换为新的ID，并将信息写入ID转换表中
void PrintInfo(unsigned short newID, int find); //打印 时间 newID 功能 域名 IP

#endif //C_MYFUNCTION_H
