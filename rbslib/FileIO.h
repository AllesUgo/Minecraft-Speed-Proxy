#pragma once
#include <iostream>
#include <exception>
#include <cstring>
#include <cerrno>
#include "BaseType.h"
#include "Buffer.h"
namespace RbsLib
{
	namespace Storage
	{
		namespace FileIO
		{
			class FileIOException :public std::exception
			{
				std::string str;
			public:
				const char* what(void)const noexcept override { return this->str.c_str(); }
				FileIOException(const std::string& str)noexcept :str(str) {}
			};
			enum class SeekBase
			{
				begin = SEEK_SET,
				end = SEEK_END,
				now = SEEK_CUR
			};
			enum class OpenMode
			{
				None = 0,
				Read = 1,/*可读*/
				Write = 1 << 1,/*可写，默认创建,已存在则追加写*/
				Replace = 1 << 2,/*清空原有文件*/
				Bin = 1 << 3/*二进制模式打开*/,
			};
			RbsLib::Storage::FileIO::OpenMode operator|(RbsLib::Storage::FileIO::OpenMode l, RbsLib::Storage::FileIO::OpenMode r)noexcept;
			RbsLib::Storage::FileIO::OpenMode operator&(RbsLib::Storage::FileIO::OpenMode l, RbsLib::Storage::FileIO::OpenMode r)noexcept;
			class File
			{
			private:
				std::FILE* fp = nullptr;
				int* quote = nullptr;
				OpenMode open_mode;
				void ThrowIfNotOpen()const;
			public:
				File(void)noexcept;
				File(const std::string& path,
					RbsLib::Storage::FileIO::OpenMode open_mode = RbsLib::Storage::FileIO::OpenMode::Read,
					RbsLib::Storage::FileIO::SeekBase default_seek = RbsLib::Storage::FileIO::SeekBase::begin,
					int64_t default_offset = 0);
				File(const RbsLib::Storage::FileIO::File& file);
				File(RbsLib::Storage::FileIO::File&& file);
				~File()noexcept;
				const RbsLib::Storage::FileIO::File& operator=(const RbsLib::Storage::FileIO::File& file)noexcept;
				const RbsLib::Storage::FileIO::File& operator=(RbsLib::Storage::FileIO::File&& file)noexcept;
				void Open(const std::string& path,
					RbsLib::Storage::FileIO::OpenMode open_mode = RbsLib::Storage::FileIO::OpenMode::Read,
					RbsLib::Storage::FileIO::SeekBase default_seek = RbsLib::Storage::FileIO::SeekBase::begin,
					int64_t default_offset = 0);
				void Close();
				template<typename T> void GetData(T& t) const
				{
					if (!fp) throw FileIOException("File is not open");
					if (!this->CheckOpenMode(RbsLib::Storage::FileIO::OpenMode::Read))
						throw FileIOException("Not have read permission");
					if (1 != fread(&t, sizeof(t), 1, fp)) throw FileIOException(std::string("Read file error:") + strerror(errno));
				}
				template<typename T> void WriteData(const T& t) const
				{
					if (!fp) throw FileIOException("File is not open");
					if (!this->CheckOpenMode(RbsLib::Storage::FileIO::OpenMode::Write))
						throw FileIOException("Not have write permission");
					if (1 != fwrite(&t, sizeof(t), 1, fp)) throw FileIOException(std::string("Write file error:") + strerror(errno));
				}
				RbsLib::Buffer Read(int64_t size)const;
				void Write(const RbsLib::IBuffer& buffer)const;
				std::string GetLine(uint64_t max_len)const;
				void WriteLine(const std::string& str)const;
				int Seek(SeekBase base, int64_t offset)const;
				bool IsOpen(void)const noexcept;
				OpenMode GetOpenMode(void)const noexcept;
				bool CheckOpenMode(OpenMode mode)const noexcept;
				bool CheckEOF(void)const noexcept;
				int GetFileDescriptor(void)const;
			};
		}
	}
}