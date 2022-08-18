# Minecraft-Speed-Proxy

Minecraft加速IP程序

## 如何编译并运行

0.安装[xmake](https://github.com/xmake-io/xmake),`bash <(curl -fsSL https://xmake.io/shget.text)`
1.克隆仓库到你的Linux服务器上
2.在Linux上安装gcc和make
    例如：`sudo apt update`
    `sudo apt install make gcc`

3.进入包含 `Makefile`的目录，输入 `make或xmake`等待编译完成
编译完成后已经程序已经可以运行，可以使用 `sudo make install`来将其放在 `/usr/bin/`目录中以在任何目录环境和用户下使用该程序

## 如何使用本程序

1.在包含 `minecraftspeedproxy`的目录输入 `./minecraftspeedproxy`可获取参数列表

#### 参数解释

1.运行本程序需要三个必选参数，依次为 `要代理的服务器地址` `要代理的服务器端口` `使用的本地端口`
其中 `要代理的服务器地址`可以是IP，也可以是域名，`使用的本地端口`请注意检查是否被占用以及权限

2.可选参数 `--noinput`用来禁用标准输入,可搭配 `nohup`命令来使其在后台运行，建议使用 `screen`来使其在后台运行而不是 `nohup`

3.运行时命令请在运行后输入 `help`以获取
