#pragma once
#include <exception>
#include "rbslib/Storage.h"
#include "rbslib/FileIO.h"
#include <list>

namespace WhiteBlackList {
	class WhiteBlackListException : public std::exception {
	protected:
		std::string message;
	public:
		WhiteBlackListException(const std::string& message) noexcept;
		virtual const char* what() const noexcept override;
	};
	void WhiteListOn();
	void WhiteListOff();
	void Init();
	bool IsWhiteListOn();
	void Init();
	void AddWhiteList(const std::string&  uuid);
	void AddBlackList(const std::string&  uuid);
	void RemoveWhiteList(const std::string&  uuid);
	void RemoveBlackList(const std::string&  uuid);
	void ClearWhiteList();
	void ClearBlackList();
	bool IsInWhite(const std::string&  uuid);
	bool IsInBlack(const std::string&  uuid);
	auto GetWhiteList() -> std::list<std::string>;
	auto GetBlackList() -> std::list<std::string>;
}