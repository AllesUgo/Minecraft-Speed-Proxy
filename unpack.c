#include "unpack.h"

int GetPackID(char *data, int datasize);
int ReadString(char *data, int datasize, char *output, int outputmaxsize,int*strlength);
int RecvFullPack(WS_Connection_t connect, char *data, int maxsize);
int ParseHandlePack(HandPack *outputpack, char *data, int maxsize);
int BuildString(const char *str, char *outputdata, int maxsize,int strlength);
int BuildHandPack(HandPack pack,const char *serveraddr,int is_fml,char*outputdata,int maxsize);
void ReadUserName(char*data,char *username);
int CheckFML(char*data);
int CheckFML(char*data)
{
    char vl,*p=data;
    varint_decode(data,1024,&vl);
    if (vl<=0) return -2;
    p+=vl;
    if (*p!=0) return -1;//不是握手包
    p+=1;
    varint_decode(p,1024,&vl);
    p+=vl;
    if (vl<=0) return -2;
    char *temp=(char*)malloc(1024);
    int strl=varint_decode(p,100,&vl);
    p+=vl;
    memcpy(temp,p,strl);
    temp[strl]=0;
    if (strl!=strlen(temp))
    {
        free(temp);
        return 1;
    }
    else
    {
        free(temp);
        return 0;
    }
}
int BuildHandPack(HandPack pack,const char *serveraddr,int is_fml,char*outputdata,int maxsize)
{
    char packsize[10];
    unsigned char packvint;
    char prosize[10];
    unsigned char provint;
    char *str=(char*)malloc(strlen(serveraddr)+20);
    int strl;
    if (is_fml==0)
    {
        strl=BuildString(serveraddr,str,strlen(serveraddr)+20,strlen(serveraddr));
    }
    else
    {
        char *temp=(char*)malloc(strlen(serveraddr)+20);
        strcpy(temp,serveraddr);
        memcpy(temp+strlen(temp)+1,"FML",4);
        strl=BuildString(temp,str,strlen(serveraddr)+20,strlen(serveraddr)+5);
        free(temp);
    }
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
    unsigned char vil;
    varint_decode(p,10,&vil);
    p+=vil;
    //获取字符串长度
    int strl=varint_decode(p,10,&vil);
    memcpy(username,p+vil,strl);
    username[strl]=0;
    return;
}

int BuildString(const char *str, char *outputdata, int maxsize,int strlength)
{
    int strl = strlength;
    unsigned char vil;
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

int ReadString(char *data, int datasize, char *output, int outputmaxsize,int *strlength)
{
    unsigned char vl;
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
    if (strlength!=NULL) *strlength=strl;
    return strl + vl;
}
int ParseHandlePack(HandPack *outputpack, char *data, int maxsize)
{
    HandPack pack;
    char *p = data;
    unsigned char lt;
    varint_decode(data, maxsize, &lt);
    
    //获取协议号
    p += lt+1;
    pack.protocol = varint_decode(p, maxsize - (p - data), &lt);
    p += lt;
    //获取源地址
    int ssize = ReadString(p, maxsize - (p - data), pack.Address, 128,NULL);
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
    unsigned char length;
    varint_decode(data, datasize, &length); //获取包大小
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
    unsigned char ss;
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