#pragma once
#include <string>
#include <cstdarg>
class Logger
{
private:
	Logger() = delete;
	static void print_log(int level, const char* log_format, va_list lst);
	static void save_log(int level, const char* log_format, va_list lst);
public:
	static bool Init(const std::string& path, int show_log_level, int save_log_level);
	static void LogInfo(const char* format, ...);
	static void LogWarn(const char* format, ...);
	static void LogError(const char* format, ...);
	static void LogPlayer(const char* format, ...);
};
class Time
{
public:
	static std::string GetFormattedTime(void)noexcept;
	static std::string ConvertTimeStampToFormattedTime(time_t time_stamp)noexcept;
};
