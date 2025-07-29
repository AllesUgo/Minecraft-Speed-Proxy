#pragma once
#include <cstdint>
#include <exception>
#include <string>

class ConfigException : public std::exception {
protected:
	std::string message;
public:
	ConfigException(const std::string& message) noexcept;
	virtual const char* what() const noexcept override;
};

class Config {
public:
	Config() = delete;
	static void SetDefaultConfig();
	static void load_config(const std::string& path);
	static void save_config(const std::string& path);
	static void set_config(const std::string& key, const std::string& value);
	static void set_config(const std::string& key, int value);
	static void set_config(const std::string& key, bool value,bool re);
	static void set_config(const std::string& key, double value);

	template <typename T>
	static T get_config(const std::string& key);
};

