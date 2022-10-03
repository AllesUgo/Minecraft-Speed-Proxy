#! /bin/bash

echo='echo -e' && [ -n "$(echo -e|grep e)" ] && echo=echo

echo "***********************************************"
echo "**                 欢迎使用                   **"
echo "**                minecraftspeedproxy-config **"
echo "**            配置文件路径默认请使用sudo         **"
echo "***********************************************"

echo -----------------------------------------------
$echo " 1 \033[32m正在配置远程服务器地址\033[0m"
echo -----------------------------------------------
read -p "请输入远程服务器地址 > " Address
if [ -z $Address ];then
	echo 已取消！ && exit 1;
fi
echo -----------------------------------------------
$echo " 2 \033[32m正在配置远程服务器端口\033[0m"
echo -----------------------------------------------
read -p "请输入远程服务器端口 > " RemotePort
if [ -z $RemotePort ];then
	echo 已取消！ && exit 1;
fi
echo -----------------------------------------------
$echo " 3 \033[32m正在配置本地监听端口\033[0m"
echo -----------------------------------------------
read -p "请输入本地监听端口 > " LocalPort
if [ -z $LocalPort ];then
	echo 已取消！ && exit 1;
fi
echo -----------------------------------------------
$echo " 3 \033[32m正在配置是否允许输入\033[0m"
echo -----------------------------------------------
read -p "请输入true or false > " AllowInput
if [ -z $AllowInput ];then
	echo 已取消！ && exit 1;
fi
echo -----------------------------------------------
$echo " 4 \033[32m正在配置配置文件路径(留空为默认)\033[0m"
echo -----------------------------------------------
read -p "请输入配置文件路径 > " configpath
if [ -z $configpath ];then
	configpath='/etc/minecraftspeedproxy/config.json'
	if [ ! -d "$configpath" ];then
		configpath='./config.json'
		touch config.json
	fi
fi
echo { \"Address\":\"$Address\", \"RemotePort\":$RemotePort, \"LocalPort\":$LocalPort, \"AllowInput\":$AllowInput } > $configpath
