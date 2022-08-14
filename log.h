#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
#include<string.h>
typedef struct TIMESTR {
    char time[128];
} TimeStr;


TimeStr gettime(void);
int saveLog(const char*);
int InitSaveLog(const char*);
