#include "CharsetConvert.h"
#include "BaseType.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef WIN32
std::string RbsLib::Encoding::CharsetConvert::UTF8toANSI(const std::string& strUTF8)
{
	UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	UINT acp = GetACP();
	nLen = WideCharToMultiByte(acp, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(acp, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	std::string str = szBuffer;
	delete[]szBuffer;
	delete[]wszBuffer;
	return str;
}

std::string RbsLib::Encoding::CharsetConvert::ANSItoUTF8(const std::string& strAnsi)
{
	UINT acp = GetACP();
	UINT nLen = MultiByteToWideChar(acp, NULL, strAnsi.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(acp, NULL, strAnsi.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	std::string str = szBuffer;
	delete[]wszBuffer;
	delete[]szBuffer;
	return str;
}

#endif

#ifdef LINUX
#include <iconv.h>
#include <cstring>
#include <stdexcept>
#include <locale.h>
#include <langinfo.h>

static std::string convert_encoding(const std::string& input, const char* from_charset, const char* to_charset)
{
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t)-1) {
        throw std::runtime_error("iconv_open failed");
    }

    size_t inbytesleft = input.size();
    size_t outbytesleft = inbytesleft * 4 + 4; // Ô¤Áô×ã¹»¿Õ¼ä
    std::string output(outbytesleft, 0);

    char* inbuf = const_cast<char*>(input.data());
    char* outbuf = &output[0];
    char* outbuf_start = outbuf;

    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1) {
        iconv_close(cd);
        throw std::runtime_error("iconv conversion failed");
    }
    iconv_close(cd);
    output.resize(outbuf - outbuf_start);
    return output;
}

std::string RbsLib::Encoding::CharsetConvert::UTF8toANSI(const std::string& strUTF8)
{
	setlocale(LC_ALL, "");
	const char* charset = nl_langinfo(CODESET);
    return convert_encoding(strUTF8, "UTF-8", charset);
}

std::string RbsLib::Encoding::CharsetConvert::ANSItoUTF8(const std::string& strAnsi)
{
	setlocale(LC_ALL, "");
	const char* charset = nl_langinfo(CODESET);
    return convert_encoding(strAnsi, charset, "UTF-8");
}
#endif
