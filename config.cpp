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

void Config::SetDefaultConfig()
{
	//�����ļ��汾���
	Config::set_config("Version", "1.0");
	//��ַ���
	Config::set_config("LocalAddress", "::");
	Config::set_config("LocalPort", 25565);

	Config::set_config("Address", "mc.hypixel.net");
	Config::set_config("RemotePort", 25565);

	//����������
	Config::set_config("MaxPlayer", -1);

	//MOTD���
	Config::set_config("MotdPath", "");

	//����
	Config::set_config("DefaultEnableWhitelist", true,true);
	Config::set_config("WhiteBlcakListPath","./WhiteBlackList.json");
	Config::set_config("AllowInput", true,true);
	Config::set_config("ShowOnlinePlayerNumber", true,true);//������

	//��־���
	Config::set_config("LogDir", "./logs");
	Config::set_config("ShowLogLevel", 0);
	Config::set_config("SaveLogLevel", 0);

}

void Config::load_config(const std::string& path)
{
	RbsLib::Storage::FileIO::File file(path);
	auto buffer = file.Read(RbsLib::Storage::StorageFile(path).GetFileSize());
	ConfigJson.Parse(buffer.ToString());
	//��鲢���������ļ�
	try
	{
		if (Config::get_config<std::string>("Version") != "1.0")
			throw ConfigException("Config version not match, need 1.0");
	}
	catch (const ConfigException& ex)
	{
		//���û�а汾�ţ���������1.0
		Config::upgrade_config_v1_0();
		std::cout << "�����ļ��汾�ϵͣ����Զ����������ļ����Ƿ񱣴棿(y/n): ";
		std::string save;
		std::cin >> save;
		getchar(); //������뻺�����Ļ��з�
		if (save != "y" && save != "Y")
		{
			std::cout << "��ȡ�����������ļ�" << std::endl;
			return;
		}
		else
		{
			std::cout << "���ڱ��������ļ�..." << std::endl;
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
