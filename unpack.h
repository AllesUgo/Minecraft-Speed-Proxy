#ifndef UNPACK
#define UNPACK
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "varint.h"
#include "websocket.h"

typedef struct HANDLEPACK
{
    int protocol;
    char username[20];
    char Address[128];
    int port;
    int nextstate;
} HandPack;

int GetPackID(char *data, int datasize);
int ReadString(char *data, int datasize, char *output, int outputmaxsize);
int RecvFullPack(WS_Connection_t connect, char *data, int maxsize);
int ParseHandlePack(HandPack *outputpack, char *data, int maxsize);
int ReadFullPack(WS_Connection_t client, char *data, int maxsize);
int BuildString(const char *str, char *outputdata, int maxsize);
void ReadUserName(char*data,char *username);
int BuildHandPack(HandPack pack,const char *serveraddr,char*outputdata,int maxsize);
#endif