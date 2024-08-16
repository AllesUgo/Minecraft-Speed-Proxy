#pragma once
#include "BaseType.h"
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstdint>
namespace RbsLib::String
{
	std::vector<std::string> split(const std::string& str, const std::string& pattern);
	namespace Convert
	{
		template <typename T>
		auto StringToNumber(const std::string& str)->T
		{
			T a = 0;
			for (auto it : str)
			{
				if (it>='0'&&it<='9')
					a = a * 10 + it - '0';
			}
			if (str.length() && str[0] == '-') return -a;
			return a;
		}

		template <typename T>
		std::string ToString(const T a_value, int n = 6)
		{
			int nn = n;
			std::ostringstream out;
			out << std::fixed << std::setprecision(nn) << a_value;
			return out.str();
		}
	}
}
