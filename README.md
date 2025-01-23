[![Build and release](https://github.com/AllesUgo/Minecraft-Speed-Proxy/actions/workflows/release.yaml/badge.svg)](https://github.com/AllesUgo/Minecraft-Speed-Proxy/actions/workflows/release.yaml)![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/AllesUgo/Minecraft-Speed-Proxy)![GitHub all releases](https://img.shields.io/github/downloads/AllesUgo/Minecraft-Speed-Proxy/total)![GitHub](https://img.shields.io/github/license/AllesUgo/Minecraft-Speed-Proxy)![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/AllesUgo/Minecraft-Speed-Proxy)
# Minecraft-Speed-Proxy

Minecraft加速IP程序  
能够代理Minecraft服务器，并拥有白名单、用户控制、流量展示、MOTD自定义等功能，支持IPv6    
~~本项目使用C语言编写~~新版已改用C++，内存占用极低，在配置较低的服务器上拥有更好的表现  
改用C++编写，大幅度降低了崩溃的发生率，降低了内存泄露的可能并有了更好的项目结构  
新版已支持跨平台编译，支持Windows、Linux  
若仍需要使用旧版，请下载v3.0.0以前的版本。若需要对旧版继续开发，请切换到c-releases分支

# 如何获取程序(Linux Ubuntu)，(Windows请参考Windows编译指南，或直接使用发行版)
### 通过发行版获取
1. 前往[Release](https://github.com/AllesUgo/Minecraft-Speed-Proxy/releases/latest)下载最新的发行版的对应版本
2. 解压下载的文件
```bash
tar -zxvf <下载的压缩包>
```
### 通过源码获取
1.克隆仓库到你的Linux服务器上  
```bash
sudo apt update
sudo apt install -y git
git clone https://github.com/AllesUgo/Minecraft-Speed-Proxy.git
```
2.在Linux上安装linux c++编译工具包
    例如：
```bash
sudo apt update
sudo apt install -y g++ cmake make 
```
3.进入包含 `CMakeList.txt`的目录编译项目,编译完成后会在项目目录下生成可执行文件
```bash
cd Minecraft-Speed-Proxy
cmake -DCMAKE_BUILD_TYPE=Release .
cmake --build .
```
# 如何获取程序(Windows)
Windows不推荐使用源码编译，编译环境较为复杂，推荐直接下载发行版  
1. 前往[Release](https://github.com/AllesUgo/Minecraft-Speed-Proxy/releases/latest)下载最新的发行版的对应版本  
2. 使用适当的解压工具解压下载的文件


# 如何使用本程序
Windows使用方法与Linux相同，在Windows下使用cmd或PowerShell代替Linux下的bash  
Linux用户可能需要先给予程序执行权限
```bash
chmod +x minecraftspeedproxy
```
在包含`minecraftspeedproxy`(`minecraftspeedproxy.exe`)程序的目录输入 `./minecraftspeedproxy -h`以获取使用帮助  
基本使用方法:  
```bash
./minecraftspeedproxy <要代理的服务器地址> <要代理的服务器端口> <使用的本地端口>
```
例如:
```bash
./minecraftspeedproxy mc.hypixel.net 25565 25565
```


## 参数解释

*本程序可以通过参数启动，也可以通过配置文件启动，若要通过配置文件启动请参考配置文件解释*  
1.通过命令行参数运行本程序需要三个必选参数，依次为 `要代理的服务器地址` `要代理的服务器端口` `使用的本地端口`
其中 `要代理的服务器地址`可以是IP，也可以是域名，`使用的本地端口`请注意检查是否被占用以及权限

3.**运行中的命令支持请在运行后输入 `help`以获取**

4.更多参数请使用参数`-h`获取  
## 配置文件解释
程序除通过命令行参数设置启动外，还可以使用配置文件启动  
配置文件是一个JSON格式的文本文件，默认的配置文件可以通过`./minecraftspeedproxy -c config.json`生成(请注意权限问题)  
其默认内容如下
```json
{
	"LocalIPv6":	false,
	"LocalAddress":	"0.0.0.0",
	"LocalPort":	25565,
	"RemoteIPv6":	false,
	"Address":	"mc.hypixel.net",
	"RemotePort":	25565,
	"MaxPlayer":	-1,
	"MotdPath":	"",
	"DefaultEnableWhitelist":	true,
	"WhiteBlcakListPath":	"./WhiteBlackList.json",
	"AllowInput":	true,
	"ShowOnlinePlayerNumber":	true,
	"LogDir":	"./logs",
	"ShowLogLevel":	0,
	"SaveLogLevel":	0
}
```  

各字段解释如下；

|键名|类型|键值|
|-|-|-|
|LocalIPv6|布尔|本机地址是否使用IPv6|
|LocalAddress|字符串|本机地址,一般填`0.0.0.0`,IPv6也可使用`::`|
|LocalPort|整数|本机端口|
|RemoteIPv6|布尔|远程服务器地址是否使用IPv6|
|Address|字符串|远程服务器地址，可以是域名或IP地址,如`mc.hypixel.net`|
|RemotePort|整数|远程服务器端口|
|MaxPlayer|整数|最大玩家数，-1为不限制|
|MotdPath|字符串|motd文件路径，为空则使用默认motd|
|DefaultEnableWhitelist|布尔|是否默认启用白名单|
|WhiteBlcakListPath|字符串|白名单黑名单文件路径|
|AllowInput|布尔|是否允许输入命令(当前版本暂时无效)|
|ShowOnlinePlayerNumber|布尔|是否显示在线玩家数(当前版本暂时无效)|
|LogDir|字符串|日志文件目录|
|ShowLogLevel|整数|显示日志等级，-1为Debug日志，0为显示常规日志，1为显示警告及以上，2为显示错误及以上，3为显示玩家状态，一般填0|
|SaveLogLevel|整数|保存日志等级，-1为Debug日志，1为保存警告及以上，2为保存错误及以上，3为保存玩家状态，一般填0|

生成配置文件请使用
```bash
./minecraftspeedproxy -a <配置文件路径>
```
使用配置文件启动请使用
```bash
./minecraftspeedproxy -c <配置文件路径>
```
例如:
```bash
./minecraftspeedproxy -c config.json
```
## 如何自定义motd
motd文件是一个文本文件，可以通过配置文件指定。若配置文件中的`MotdPath`字段为空则使用默认motd
以下是一个motd文件的例子
```JSON
{
    "version": {
        "name": "1.8.7",
        "protocol": 47
    },
    "players": {
        "max": 100,
        "online": 5,
        "sample": [
            {
                "name": "thinkofdeath",
                "id": "4566e69f-c907-48ee-8d71-d7ba5aa00d20"
            }
        ]
    },    
    "description": {
        "text": "Hello world"
    },
    "favicon": "data:image/png;base64,<data>"
}
```
MOTD必须是一个JSON格式的文本文件，其中的字段均可空或部分为空，当某字段为空时，服务器将会使用默认值填充该字段  
例如，当players字段为空时，服务器将会使用设置的最大玩家数、在线玩家数和玩家列表填充该字段  
若希望获取现有服务器的motd数据，可以使用命令`./minecraftspeedproxy --get-motd`并按照提示操作

## 二次开发
二次开发文档请参考: [二次开发](SecondaryDevelopment.md)