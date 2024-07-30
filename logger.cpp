#include "logger.h"
#include <time.h>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <filesystem>
#include <mutex>

std::string LogDir;
std::mutex LogMutex;
FILE* Fp = nullptr;
std::string LogFileName;
bool IsInit = false;
int ShowLogLevel = 0;
int SaveLogLevel;
using namespace std;



static void WriteFrmtd(FILE* stream, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stream, format, args);
	va_end(args);
}
void Logger::print_log(int level, const char* log_format, va_list lst)
{
	if (level >= ShowLogLevel)
		vprintf(log_format, lst);
}
void Logger::save_log(int level, const char* log_format, va_list lst)
{
	std::unique_lock<std::mutex> lock(LogMutex);
	if (IsInit && level >= SaveLogLevel)
	{
		char tmp[64];
		struct tm* timinfo;
		time_t time_stamp = time(nullptr);
		timinfo = localtime(&time_stamp);
		strftime(tmp, sizeof(tmp), "%Y-%m-%d", timinfo);
		if (LogFileName == string(tmp) && Fp)
		{
			vfprintf(Fp, log_format, lst);
			fflush(Fp);
		}
		else
		{
			if (Fp) fclose(Fp);
			LogFileName = string(tmp);
			Fp = fopen((LogDir + "/" + LogFileName + ".log").c_str(), "a");
			if (Fp) vfprintf(Fp, log_format, lst), fflush(Fp);
		}
	}
}
bool Logger::Init(const std::string& path, int show_log_level, int save_log_level)
{
	if (path.empty()) return false;
	ShowLogLevel = show_log_level;
	SaveLogLevel = save_log_level;
	//检测目标文件是否存在
	if (std::filesystem::exists(path) == false)
		if (false == std::filesystem::create_directory(path)) return false;
	//检测目标文件类型
	if (std::filesystem::status(path).type() == std::filesystem::file_type::directory)
	{
		LogDir = path;
		IsInit = true;
		return true;
	}
	else
	{
		IsInit = false;
		return false;
	}
}
void Logger::LogInfo(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [INFO] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	Logger::print_log(0, (std::string("\033[0m") + std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
	va_start(lst, format);
	Logger::save_log(0, (std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
}

void Logger::LogWarn(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [Warn] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	Logger::print_log(1, (std::string("\033[0m\033[33m") + std::string(tmp) + format + "\033[0m" + "\n").c_str(), lst);
	va_end(lst);
	va_start(lst, format);
	Logger::save_log(1, (std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
}

void Logger::LogError(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [ERROR] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	Logger::print_log(2, (std::string("\033[0m\033[31m") + std::string(tmp) + format + "\033[0m" + "\n").c_str(), lst);
	va_end(lst);
	va_start(lst, format);
	Logger::save_log(2, (std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
}

void Logger::LogPlayer(const char* format, ...)
{
	va_list lst;
	char tmp[80];
	sprintf(tmp, "[%s] [Player] ", Time::GetFormattedTime().c_str());
	va_start(lst, format);
	Logger::print_log(3, (std::string("\033[0m\033[35m") + std::string(tmp) + format + "\033[0m" + "\n").c_str(), lst);
	va_end(lst);
	va_start(lst, format);
	Logger::save_log(3, (std::string(tmp) + format + "\n").c_str(), lst);
	va_end(lst);
}

std::string Time::GetFormattedTime(void) noexcept
{
	return Time::ConvertTimeStampToFormattedTime(time(0));
}

std::string Time::ConvertTimeStampToFormattedTime(time_t time_stamp) noexcept
{
	char tmp[64];
	struct tm* timinfo;
	timinfo = localtime(&time_stamp);

	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H-%M-%S", timinfo);
	return tmp;
}
