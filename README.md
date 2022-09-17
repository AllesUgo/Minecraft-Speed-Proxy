[![Build and release](https://github.com/AllesUgo/Minecraft-Speed-Proxy/actions/workflows/release.yaml/badge.svg)](https://github.com/AllesUgo/Minecraft-Speed-Proxy/actions/workflows/release.yaml)![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/AllesUgo/Minecraft-Speed-Proxy)![GitHub all releases](https://img.shields.io/github/downloads/AllesUgo/Minecraft-Speed-Proxy/total)![GitHub](https://img.shields.io/github/license/AllesUgo/Minecraft-Speed-Proxy)![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/AllesUgo/Minecraft-Speed-Proxy)
# Minecraft-Speed-Proxy

Minecraft加速IP程序

# 生成配置文件

```bash
bash <(curl -fsSL https://fastly.jsdelivr.net/gh/AllesUgo/Minecraft-Speed-Proxy@master/scripts/config.sh )
```

# 如何编译并运行
## 通过make命令安装
1.克隆仓库到你的Linux服务器上  
2.在Linux上安装gcc和make  
    例如：
    
    sudo apt update
    sudo apt install make gcc
3.进入包含 `Makefile`的目录，输入 `make`等待编译完成


4.(可选步骤)编译完成后已经程序已经可以运行，可以使用 `sudo make install`来将其放在 `/usr/bin/`目录中以在任何目录环境和用户下使用该程序
## 通过xmake安装
1.安装[xmake](https://github.com/xmake-io/xmake) ,`bash <(curl -fsSL https://xmake.io/shget.text)`, 或参考[xmake.io](https://xmake.io)使用更多方式安装  
2.克隆仓库到你的Linux服务器上  
3.在Linux上安装gcc
    例如：
    
    sudo apt update
    sudo apt install gcc

4.获取最新version `version=$(git describe --tags) && version1=${version#v} && export VERSION=${version1%%-*}`

5.进入包含 `xmake.lua`的目录，输入 `xmake`等待编译完成，生成的文件在`build`目录下寻找

6.(可选步骤)编译完成后已经程序已经可以运行，可以使用 `xmake install --admin`来将其放在 `/usr/bin/`目录中以在任何目录环境和用户下使用该程



# 如何使用本程序

在包含 `minecraftspeedproxy`的目录输入 `./minecraftspeedproxy --help`以获取参数列表

若通过make install安装或通过deb包安装则直接输入`minecraftspeedproxy --help`以获取参数列表

## 参数解释

1.运行本程序需要三个必选参数，依次为 `要代理的服务器地址` `要代理的服务器端口` `使用的本地端口`
其中 `要代理的服务器地址`可以是IP，也可以是域名，`使用的本地端口`请注意检查是否被占用以及权限

2.可选参数 `--noinput`用来禁用标准输入,可搭配 `nohup`命令来使其在后台运行，建议使用 `screen`来使其在后台运行而不是 `nohup`

3.运行中的命令支持请在运行后输入 `help`以获取

4.更多参数请使用参数`--help`获取

## 使用自定义登录检查组件
程序支持自己实现对登录用户的检查，只需要简单的编写一个动态链接库即可实现

动态库文件名**必须**为`check.so`,要求**必须**包含`check`函数，函数原型为

`int check(const char*username)`

当用户登入时会调用该函数，将登入的用户名作为参数传递给动态链接库中的check函数，check通过返回值告知服务器是否允许登录
### 返回值含义
|返回值|含义|在客户端上的提示|可否登入|
|------|---|---------------|-------|
|0|登录成功|无|是|
|1|玩家不在白名单中|服务器已开启白名单|否|
|2|玩家被服务器封禁|您已被加速服务器封禁|否|
|其他|未定义的返回值|代理服务器拒绝连接|否|

### 注意事项
1.服务器成功找到并加载该组件后内置的白名单、黑名单将会失效

2.编写该组件时无需考虑多线程互斥问题，服务器保证check函数不会同时被调用

3.组件运行中出现错误，例如段错误等会导致服务器不稳定或崩溃，编写插件时请注意

4.组件会在服务器启动时加载一次，运行过程中不会重新加载组件，若要重新加载组件请重新启动服务器

5.请注意检查组件是否具有可执行权限
### 代码示例
以下代码禁止名称为`Steve`的玩家登录服务器

```c
//check.c
#include<stdio.h>
#include<string.h>

int check(const char*username)
{
    if (!strcmp(username,"Steve"))
    {
        return 2;//被封禁
    }
    return 0;
}
```
编译方法:
```bash
gcc check.c -fPIC -shared -o check.so
```
### 组件的加载方法
将组件命名为check.so，放在**运行服务器时命令行所在目录**,启动服务器时若该组件可用会自动加载该组件
