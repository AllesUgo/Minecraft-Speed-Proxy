#include "config.h"
#include "json/CJsonObject.h"
#include "rbslib/FileIO.h"
#include "rbslib/Storage.h"

static neb::CJsonObject ConfigJson;
static void ThrowIfConfigItemNotExist(const std::string& key) {
	if (!ConfigJson.KeyExist(key)) 
	{
		throw ConfigException(std::string("Config option ")+key+" not exist");
	}
}
// Template specialization for get_config
template <>
std::string Config::get_config<std::string>(const std::string& key) {
	// Implementation for getting string config value
	ThrowIfConfigItemNotExist(key);
	std::string value;
	ConfigJson.Get(key, value);
	return value;
}

template <>
int Config::get_config<int>(const std::string& key) {
	// Implementation for getting int config value
	ThrowIfConfigItemNotExist(key);
	int value;
	ConfigJson.Get(key, value);
	return value;
}

template <>
bool Config::get_config<bool>(const std::string& key) {
	// Implementation for getting bool config value
	ThrowIfConfigItemNotExist(key);
	bool value;
	ConfigJson.Get(key, value);
	return value;
}

template <>
double Config::get_config<double>(const std::string& key) {
	// Implementation for getting double config value
	ThrowIfConfigItemNotExist(key);
	double value;
	ConfigJson.Get(key, value);
	return value;
}

ConfigException::ConfigException(const std::string& message) noexcept
	: message(message)
{
}

const char* ConfigException::what() const noexcept
{
	return message.c_str();
}

void Config::upgrade_config_v1_0()
{
	try
	{
		Config::get_config<std::string>("Version");
	}
	catch (const ConfigException& ex)
	{
		Config::set_config("Version", "1.0");
		ConfigJson.Delete("LocalIPv6");
		ConfigJson.Delete("RemoteIPv6");
	}
}

void Config::upgrade_config_v1_1()
{
	if (Config::get_config<std::string>("Version") == "1.0")
	{
		Config::set_config("Version", "1.1");
		Config::set_config("WebAPIEnable", true,true);
		Config::set_config("WebAPIAddress", "127.0.0.1");
		Config::set_config("WebAPIPort", 20220);
		Config::set_config("WebAPIPassword", "admin");
	}
}

void Config::SetDefaultConfig()
{
	//配置文件版本相关
	Config::set_config("Version", "1.1");
	//地址相关
	Config::set_config("LocalAddress", "::");
	Config::set_config("LocalPort", 25565);

	Config::set_config("Address", "mc.hypixel.net");
	Config::set_config("RemotePort", 25565);

	//玩家限制相关
	Config::set_config("MaxPlayer", -1);

	//MOTD相关
	Config::set_config("MotdPath", "");

	//杂项
	Config::set_config("DefaultEnableWhitelist", true,true);
	Config::set_config("WhiteBlcakListPath","./WhiteBlackList.json");
	Config::set_config("AllowInput", true,true);
	Config::set_config("ShowOnlinePlayerNumber", true,true);//已弃用

	//日志相关
	Config::set_config("LogDir", "./logs");
	Config::set_config("ShowLogLevel", 0);
	Config::set_config("SaveLogLevel", 0);

	//WebAPI相关
	Config::set_config("WebAPIEnable", true,true);
	Config::set_config("WebAPIAddress", "127.0.0.1");
	Config::set_config("WebAPIPort", 20220);
	Config::set_config("WebAPIPassword", "admin");

}

void Config::load_config(const std::string& path)
{
	RbsLib::Storage::FileIO::File file(path);
	auto buffer = file.Read(RbsLib::Storage::StorageFile(path).GetFileSize());
	ConfigJson.Parse(buffer.ToString());
	//检查并升级配置文件
	try
	{
		if (Config::get_config<std::string>("Version") != "1.1")
			throw ConfigException("Config version not match, need 1.1");
	}
	catch (const ConfigException& ex)
	{
		//如果没有版本号，则升级到1.0
		Config::upgrade_config_v1_0();
		Config::upgrade_config_v1_1();
		std::cout << "Config file version is low, auto-upgraded config file. Do you want to save it? (y/n): ";
		std::string save;
		std::cin >> save;
		getchar(); //清除输入缓存中的换行符
		if (save != "y" && save != "Y")
		{
			std::cout << "Canceled saving upgraded config file" << std::endl;
			return;
		}
		else
		{
			std::cout << "Saving upgraded config file..." << std::endl;
			Config::save_config(path);
		}
	}
	
}

void Config::save_config(const std::string& path)
{
	RbsLib::Storage::FileIO::File file(path,RbsLib::Storage::FileIO::OpenMode::Write|RbsLib::Storage::FileIO::OpenMode::Replace);
	file.Write(RbsLib::Buffer(ConfigJson.ToFormattedString()));
}

void Config::set_config(const std::string& key, const std::string& value)
{
	ConfigJson.ReplaceAdd(key, value);
}

void Config::set_config(const std::string& key, int value)
{
	ConfigJson.ReplaceAdd(key, value);
}

void Config::set_config(const std::string& key, bool value,bool re)
{
	ConfigJson.Delete(key);
	ConfigJson.Add(key,value,value);
}

void Config::set_config(const std::string& key, double value)
{
	ConfigJson.ReplaceAdd(key, value);
}
