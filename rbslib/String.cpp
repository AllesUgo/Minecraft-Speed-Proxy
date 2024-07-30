#include "String.h"
#include <string>
#include <cstring>
#include <vector>

std::vector<std::string> RbsLib::String::split(const std::string& str, const std::string& pattern)
{
	using namespace std;
	char* strc = new char[str.length()];
	strcpy(strc, str.c_str());
	std::vector<std::string> res;
	char* temp = strtok(strc, pattern.c_str());
	while (temp != NULL)
	{
		res.push_back(std::string(temp));
		temp = strtok(NULL, pattern.c_str());
	}
	delete[] strc;
	return res;
}
