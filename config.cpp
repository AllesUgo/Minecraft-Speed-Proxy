#include "config.h"
#include "json/CJsonObject.h"
#include "rbslib/FileIO.h"
#include "rbslib/Storage.h"

static neb::CJsonObject ConfigJson;
static void ThrowIfConfigItemNotExist(const std::string& key) {
	if (ConfigJson.IsEmpty()) {
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

void Config::SetDefaultConfig()
{
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

}

void Config::load_config(const std::string& path)
{
	RbsLib::Storage::FileIO::File file(path);
	auto buffer = file.Read(RbsLib::Storage::StorageFile(path).GetFileSize());
	ConfigJson.Parse(buffer.ToString());
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
