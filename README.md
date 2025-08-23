[![Build and release](https://github.com/AllesUgo/Minecraft-Speed-Proxy/actions/workflows/release.yaml/badge.svg)](https://github.com/AllesUgo/Minecraft-Speed-Proxy/actions/workflows/release.yaml)
![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/AllesUgo/Minecraft-Speed-Proxy)
![GitHub all releases](https://img.shields.io/github/downloads/AllesUgo/Minecraft-Speed-Proxy/total)
![GitHub](https://img.shields.io/github/license/AllesUgo/Minecraft-Speed-Proxy)
![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/AllesUgo/Minecraft-Speed-Proxy)

# Minecraft-Speed-Proxy

Minecraft加速IP程序，支持代理Minecraft服务器，拥有白名单、用户控制、流量展示、MOTD自定义等功能，支持IPv6与Forge客户端。允许通过Web API控制服务器。

新版采用C++开发，内存占用低，崩溃率和内存泄漏风险大幅降低，结构更优。使用异步IO大幅提高吞吐量。支持跨平台编译（Windows、Linux）。如需旧版（C语言），请下载v3.0.0以前版本或切换到c-releases分支。

---

## 获取方式

### 发行版下载
1. 前往 [Release](https://github.com/AllesUgo/Minecraft-Speed-Proxy/releases/latest) 下载最新版本
2. 解压文件
   ```bash
   tar -zxvf <下载的压缩包>
   ```

### 源码编译（Linux）
>此处以试用Ubuntu为例，其他Linux发行版请自行安装相应依赖。
1. 克隆仓库
   ```bash
   sudo apt update
   sudo apt install -y git
   git clone https://github.com/AllesUgo/Minecraft-Speed-Proxy.git
   ```
2. 安装编译工具
   ```bash
   sudo apt install -y g++ cmake make
   ```
3. 编译项目
   ```bash
   cd Minecraft-Speed-Proxy
   cmake --preset=linux-release
   cd out/build/linux-release
   cmake --build .
   ```

### Windows
>编译环境较大，建议直接下载发行版。
1. 安装 [Visual Studio 2022](https://visualstudio.microsoft.com/zh-hans/downloads/)（需包含C++组件）
2. 打开Visual Studio后选择克隆存储库，并填入仓库地址克隆`https://github.com/AllesUgo/Minecraft-Speed-Proxy.git`
3. 完成后直接编译运行
---

## 使用方法

### 启动
Linux需先赋予执行权限(*一般已拥有*)：
```bash
chmod +x minecraftspeedproxy
```

在程序目录下运行：
```bash
./minecraftspeedproxy -h
```

基本命令：
```bash
./minecraftspeedproxy <服务器地址> <服务器端口> <本地端口>
```
示例：
```bash
./minecraftspeedproxy mc.hypixel.net 25565 25565
```

### 参数说明
- 必选参数：`服务器地址` `服务器端口` `本地端口`
- 更多参数：`-h` 查看帮助
- 运行后输入 `help` 获取命令支持

---

## 配置文件说明

支持命令行参数或配置文件启动。
生成配置文件：
```bash
./minecraftspeedproxy -a <配置文件路径>
```
使用配置文件启动：
```bash
./minecraftspeedproxy -c <配置文件路径>
```
>配置文件路径需要包含文件名，如`./config.json`。

默认配置文件内容如下：
```json
{
	"Version": "1.1",
	"LocalAddress": "::",
	"LocalPort": 25565,
	"Address": "mc.hypixel.net",
	"RemotePort": 25565,
	"MaxPlayer": -1,
	"MotdPath": "",
	"DefaultEnableWhitelist": true,
	"WhiteBlcakListPath": "./WhiteBlackList.json",
	"AllowInput": true,
	"ShowOnlinePlayerNumber": true,
	"LogDir": "./logs",
	"ShowLogLevel": 0,
	"SaveLogLevel": 0,
    "WebAPIEnable": 1,
	"WebAPIAddress": "127.0.0.1",
	"WebAPIPort": 8080,
	"WebAPIPassword": "admin"
}
```

| 键名 | 类型 | 说明 |
|---|---|---|
| Version | 字符串 | 配置文件版本号 |
| LocalIPv6 | 布尔 | 本机地址是否使用IPv6 |
| LocalAddress | 字符串 | 本机地址（如`0.0.0.0`或`::`） |
| LocalPort | 整数 | 本机端口 |
| RemoteIPv6 | 布尔 | 远程服务器地址是否使用IPv6 |
| Address | 字符串 | 远程服务器地址（域名或IP） |
| RemotePort | 整数 | 远程服务器端口 |
| MaxPlayer | 整数 | 最大玩家数，-1不限制 |
| MotdPath | 字符串 | motd文件路径，空则默认 |
| DefaultEnableWhitelist | 布尔 | 是否默认启用白名单 |
| WhiteBlcakListPath | 字符串 | 白/黑名单文件路径 |
| AllowInput | 布尔 | 是否允许输入命令 |
| ShowOnlinePlayerNumber | 布尔 | 是否显示在线玩家数（暂未实现） |
| LogDir | 字符串 | 日志目录 |
| ShowLogLevel | 整数 | 显示日志等级 |
| SaveLogLevel | 整数 | 保存日志等级 |
| WebAPIEnable | 布尔 | 是否启用Web API |
| WebAPIAddress | 字符串 | Web API监听地址 |
| WebAPIPort | 整数 | Web API监听端口 |
| WebAPIPassword | 字符串 | Web API访问密码 |

---

## WebAPI

WebAPI允许通过HTTP请求控制服务器，包含控制台全部功能及扩展。
默认启用，可在配置文件中设置`WebAPIEnable`为`0`关闭。
接口文档详见 [WebAPI.md]。
> [!WARNING]
> 你的密码将被不加密传输，请慎重考虑启用WebAPI及网络环境安全性。可以使用反向代理用HTTPS提高安全性。
---

## MOTD自定义

motd文件为JSON格式文本，可通过配置文件指定路径。
示例：
```json
{
    "version": {"name": "1.8.7", "protocol": 47},
    "players": {"max": 100, "online": 5, "sample": [{"name": "thinkofdeath", "id": "4566e69f-c907-48ee-8d71-d7ba5aa00d20"}]},
    "description": {"text": "Hello world"},
    "favicon": "data:image/png;base64,<data>"
}
```
字段可为空，服务器将自动填充默认值。  
获取现有服务器motd数据：
```bash
./minecraftspeedproxy --get-motd
```

---

## 二次开发

详见 [二次开发](SecondaryDevelopment.md)。