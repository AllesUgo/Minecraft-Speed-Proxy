#include "log.h"
pthread_mutex_t Log_File_RW_LOCK; //防止同时修改同一个文件
char Log_Path[256];
int DAY = 0;
int initerror = 1;
FILE *LogFile = NULL;
TimeStr gettime(void)
{
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);

    info = localtime(&rawtime);
    TimeStr surstr;
    strftime(surstr.time, 128, "%Y-%m-%d %H:%M:%S", info);
    //printf("[%s]", buffer );
    return surstr;
}

int InitSaveLog(const char *path)
{
    strncpy(Log_Path, path, 256);
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);

    info = localtime(&rawtime);
    char temp[388];
    strftime(temp, 5, "%d", info);
    sscanf(temp, "%d", &DAY); //记录当前日期
    //构建log文件名
    char logday[20];
    strftime(logday, 128, "%Y-%m-%d", info);
    sprintf(temp, "%s/%s.log", path, logday);
    //打开该文件
    LogFile = fopen(temp, "a");
    if (LogFile == NULL)
    {
        initerror = 1;
        return -1;
    }
    initerror = 0;
    return 0;
}
int saveLog(const char *str)
{
    if (initerror != 0)
    {
        return -2; //没有初始化或者初始化失败
    }
    //线程加锁
    pthread_mutex_lock(&Log_File_RW_LOCK);
    //先检查日期是否需要更新
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);

    info = localtime(&rawtime);
    char temp[388];
    strftime(temp, 5, "%d", info);
    int day;
    sscanf(temp, "%d", &day); //读取当前日期
    //对比记录的日期
    if (day != DAY)
    {
        //日期已经变更
        DAY = day;
        fclose(LogFile);
        //构建新的路径
        //构建log文件名
        char logday[20];
        strftime(logday, 128, "%Y-%m-%d", info);
        sprintf(temp, "%s/%s.log", Log_Path, logday);
        //打开该文件
        LogFile = fopen(temp, "a");
        if (LogFile == NULL)
        {
            pthread_mutex_unlock(&Log_File_RW_LOCK); //打开失败
            return -1;
        }
    }
    //记录日志
    fprintf(LogFile, "%s\n", str);
    fflush(LogFile);
    pthread_mutex_unlock(&Log_File_RW_LOCK);
    return 0;
}
