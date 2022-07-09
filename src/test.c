//
// Created by LaoKaShouJi on 2022/7/6.
//
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

int main(int argc, char const *argv[])
{
    char timestr[128]="";
    time_t now;
    time(&now);
    strftime(timestr, 128, "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("%s\r\n", timestr);

    return 0;
}