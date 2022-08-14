#include "unpack.h"

int GetPackID(char *data, int datasize);
int ReadString(char *data, int datasize, char *output, int outputmaxsize);
int RecvFullPack(WS_Connection_t connect, char *data, int maxsize);
int ParseHandlePack(HandPack *outputpack, char *data, int maxsize);
int BuildString(const char *str, char *outputdata, int maxsize);
int BuildHandPack(HandPack pack,const char *serveraddr,char*outputdata,int maxsize);
void ReadUserName(char*data,char *username);
int BuildHandPack(HandPack pack,const char *serveraddr,char*outputdata,int maxsize)
{
    char packsize[10];
    char packvint;
    char prosize[10];
    char provint;
    char *str=(char*)malloc(strlen(serveraddr)+10);
    int strl=BuildString(serveraddr,str,strlen(serveraddr)+10);
    varint_encode(pack.protocol,prosize,10,&provint);
    //构建数据包
    char *data=(char*)malloc(1024);
    char*p=data;
    *p=0;
    p+=1;
    memcpy(p,prosize,provint);
    p+=provint;
    memcpy(p,str,strl);
    p+=strl;
    short port=(short)htons(pack.port);
    memcpy(p,&port,2);
    p+=2;
    *p=2;
    p+=1;
    varint_encode(p-data,packsize,10,&packvint);
    memcpy(outputdata,packsize,packvint);
    memcpy(outputdata+packvint,data,p-data);
    free(data);
    free(str);
    return p-data+packvint;
}





void ReadUserName(char*data,char *username)
{
    char *p=data;
    char vil;
    varint_decode(p,10,&vil);
    p+=vil;
    //获取字符串长度
    int strl=varint_decode(p,10,&vil);
    memcpy(username,p+vil,strl);
    username[strl]=0;
    return;
}

int BuildString(const char *str, char *outputdata, int maxsize)
{
    int strl = strlen(str);
    char vil;
    char temp[10];
    varint_encode(strl, temp, 10, &vil);
    if (vil + strl > maxsize)
    {
        return -1;
    }
    memcpy(outputdata,temp,vil);
    memcpy(outputdata+vil,str,strl);
    return vil+strl;
}

int ReadString(char *data, int datasize, char *output, int outputmaxsize)
{
    char vl;
    int strl = varint_decode(data, datasize, &vl);
    if (vl + strl > datasize)
    {
        return -1;
    }
    if (outputmaxsize < strl + 1)
    {
        return -2;
    }
    memcpy(output, data + vl, strl);
    output[strl] = 0;
    return strl + vl;
}
int ParseHandlePack(HandPack *outputpack, char *data, int maxsize)
{
    HandPack pack;
    char *p = data;
    char lt;
    int packsize = varint_decode(data, maxsize, &lt);
    
    //获取协议号
    p += lt+1;
    pack.protocol = varint_decode(p, maxsize - (p - data), &lt);
    p += lt;
    //获取源地址
    int ssize = ReadString(p, maxsize - (p - data), pack.Address, 128);
    if (ssize <= 0)
    {
        return -1;
    }
    p += ssize;
    //读取源端口
    memcpy(&(pack.port), p, 2);
    p += 2;
    //转换字节顺序
    pack.port = ntohs(pack.port);
    pack.nextstate=*p;
    *outputpack = pack;
    return 0;
}
int GetPackID(char *data, int datasize)
{
    //获取包长度
    char *p = data;
    char length;
    int packsize = varint_decode(data, datasize, &length); //获取包大小
    datasize -= length;
    if (datasize <= 0)
    {
        return -10000;
    }
    //获取包ID
    return varint_decode(data + length, datasize, NULL);
}
int RecvFullPack(WS_Connection_t client, char *data, int maxsize)
{
    
    //先接收10字节数据来看包大小
    char pack[128];
    int i = WS_Recv(&client, pack, 10);
    if (i <= 0)
    {
        return -1;
    }
    char ss;
    int packsize = varint_decode(pack, 2, &ss); //获取数据包大小
    //继续读取完整数据包
    int needread = ss + packsize; //整个数据包大小
    if (needread <= i)
    {
        if (needread <= maxsize)
        {
            memcpy(data, pack, i);
            return needread;
        }
        else
        {
            return -2;
        }
    }
    int needreadnext = needread - i; //剩余需要读取的大小
    if (needreadnext > maxsize)
    {
        return -2;
    }
    memcpy(data, pack, i);
    char *p = data + i;
    int re=0;
    while (needreadnext > 0&&needreadnext<maxsize)
    {
        re = WS_Recv(&client, p, needreadnext);
        if (re <= 0||re>=maxsize)
        {
            return -3;
        }
        p += re;
        needreadnext -= re;
    }
    return needread;
}