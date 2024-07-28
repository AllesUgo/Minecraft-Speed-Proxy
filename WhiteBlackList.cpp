#include "WhiteBlackList.h"
#include <mutex>
#include "config.h"
#include "json/CJsonObject.h"

static std::list<std::string> WhiteList,BlackList;
static std::mutex WhiteBlackListMutex;
static bool WhiteListStatus = false;

/*本函数不做多线程保护*/
static void SaveWhiteBlackList()
{
	neb::CJsonObject config;
	config.AddEmptySubArray("WhiteList");
	config.AddEmptySubArray("BlackList");
	for (auto& uuid : WhiteList)
	{
		config["WhiteList"].Add(uuid);
	}
	for (auto& uuid : BlackList)
	{
		config["BlackList"].Add(uuid);
	}
	RbsLib::Storage::StorageFile file(Config::get_config<std::string>("WhiteBlcakListPath"));
	file.Open(RbsLib::Storage::FileIO::OpenMode::Write| RbsLib::Storage::FileIO::OpenMode::Replace, RbsLib::Storage::FileIO::SeekBase::begin, 0L).Write(RbsLib::Buffer(config.ToFormattedString()));
}

void WhiteBlackList::WhiteListOn()
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	WhiteListStatus = true;
}

void WhiteBlackList::WhiteListOff()
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	WhiteListStatus = false;
}



void WhiteBlackList::Init()
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	neb::CJsonObject config;
	RbsLib::Storage::StorageFile file(Config::get_config<std::string>("WhiteBlcakListPath"));
	if (!file.IsExist())
	{
		file.Open(RbsLib::Storage::FileIO::OpenMode::Write, RbsLib::Storage::FileIO::SeekBase::begin, 0L).Write(RbsLib::Buffer("{\"WhiteList\":[],\"BlackList\":[]}"));
	}
	auto buffer = file.Open(RbsLib::Storage::FileIO::OpenMode::Read,RbsLib::Storage::FileIO::SeekBase::begin,0L).Read(file.GetFileSize());
	if (!config.Parse(buffer.ToString()))
	{
		throw WhiteBlackListException("WhiteBlackList config file parse failed");
	}
	for (int i=0;i<config["WhiteList"].GetArraySize();i++)
	{
		WhiteList.push_back(config["WhiteList"](i));
	}
	for (int i = 0; i < config["BlackList"].GetArraySize(); i++)
	{
		BlackList.push_back(config["BlackList"](i));
	}
	lock.unlock();
	Config::get_config<bool>("DefaultEnableWhitelist") ? WhiteListOn() : WhiteListOff();
}

void WhiteBlackList::AddWhiteList(const std::string& uuid)
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	if (std::find(WhiteList.begin(), WhiteList.end(), uuid) != WhiteList.end())
	{
		throw WhiteBlackListException("User already in WhiteList");
	}
	WhiteList.push_back(uuid);
	SaveWhiteBlackList();
}

void WhiteBlackList::AddBlackList(const std::string& uuid)
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	if (std::find(BlackList.begin(), BlackList.end(), uuid) != BlackList.end())
	{
		throw WhiteBlackListException("User already in BlackList");
	}
	BlackList.push_back(uuid);
	SaveWhiteBlackList();
}

void WhiteBlackList::RemoveWhiteList(const std::string& uuid)
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	auto iterator = std::find(WhiteList.begin(), WhiteList.end(), uuid);
	if (iterator == WhiteList.end())
	{
		throw WhiteBlackListException("User not in WhiteList");
	}
	WhiteList.erase(iterator);
	SaveWhiteBlackList();
}

void WhiteBlackList::RemoveBlackList(const std::string& uuid)
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	auto iterator = std::find(BlackList.begin(), BlackList.end(), uuid);
	if (iterator == BlackList.end())
	{
		throw WhiteBlackListException("uuid not in BlackList");
	}
	BlackList.erase(iterator);
	SaveWhiteBlackList();
}

void WhiteBlackList::ClearWhiteList()
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	WhiteList.clear();
	SaveWhiteBlackList();
}

void WhiteBlackList::ClearBlackList()
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	BlackList.clear();
	SaveWhiteBlackList();
}

bool WhiteBlackList::IsInWhite(const std::string& uuid)
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	if (WhiteListStatus == false) return true;
	return std::find(WhiteList.begin(), WhiteList.end(), uuid) != WhiteList.end();
}

bool WhiteBlackList::IsInBlack(const std::string& uuid)
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	return std::find(BlackList.begin(), BlackList.end(), uuid) != BlackList.end();
}

auto WhiteBlackList::GetWhiteList() -> std::list<std::string>
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	return WhiteList;
}

auto WhiteBlackList::GetBlackList() -> std::list<std::string>
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	return BlackList;
}

bool WhiteBlackList::IsWhiteListOn()
{
	std::unique_lock<std::mutex> lock(WhiteBlackListMutex);
	return WhiteListStatus;
}


WhiteBlackList::WhiteBlackListException::WhiteBlackListException(const std::string& message) noexcept
	: message(message)
{
}

const char* WhiteBlackList::WhiteBlackListException::what() const noexcept
{
	return message.c_str();
}
