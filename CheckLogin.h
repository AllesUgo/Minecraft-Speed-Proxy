#ifndef CHECKLOGIN
#define CHECKLOGIN
#define CHECK_LOGIN_SUCCESS 0
#define CHECk_LOGIN_NOT_ON_WHITELIST 1
#define CHECK_LOGIN_BANNED 2
typedef struct LINK {
    char*username;
    struct LINK*next;
} Link_t;
int CL_Check(const char *username);
int CL_EnableWhiteList();
int CL_DisabledWhiteList();
int CL_ReloadWhiteList();
int CL_LoadBanList();
int CL_WhiteListAdd(const char*playername);
int CL_WhiteListRemove(const char*playername);
int CL_BanListAdd(const char*playername);
int CL_BanListRemove(const char*playername);
#endif