#pragma once
#include <string>

namespace RbsLib::Encoding::CharsetConvert
{
	std::string UTF8toANSI(const std::string& utf8Str);
	std::string ANSItoUTF8(const std::string& ansiStr);
} // namespace RbsLib::Encoding::CharsetConvert
