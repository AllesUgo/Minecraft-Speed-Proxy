#pragma once
#include<string>
#include<filesystem>
#include<exception>
#include "BaseType.h"
namespace RbsLib
{
	namespace Storage
	{
		namespace FileType
		{
			enum FileType
			{
				Dir,
				Regular,
				Other
			};
		}

		enum class FilePermission
		{
			None = 0,
			OwnerRead = 1,
			OwnerWrite = 1 << 1,
			OwnerExecute = 1 << 2,
			GroupRead = 1 << 3,
			GroupWrite = 1 << 4,
			GroupExecute = 1 << 5,
			OtherRead = 1 << 6,
			OtherWrite = 1 << 7,
			OtherExecute = 1 << 8,
			Read = 1 << 9,
			Write = 1 << 10,
			Execute = 1 << 11
		};
		RbsLib::Storage::FilePermission operator|(FilePermission left, FilePermission right)noexcept;
		RbsLib::Storage::FilePermission operator|=(FilePermission& left, FilePermission right)noexcept;
		RbsLib::Storage::FilePermission operator&(FilePermission left, FilePermission right)noexcept;
		RbsLib::Storage::FilePermission operator&=(FilePermission& left, FilePermission right)noexcept;

		class StorageException :public std::exception
		{
		private:
			std::string str;
		public:

			enum Reason
			{
				FileAbsent,
				FileTypeError,
				FilePermissionError
			} reason;
			StorageException(const std::string& error, Reason reason) :str(error), reason(reason) {}
			const char* what(void)const noexcept override { return this->str.c_str(); }
		};
		namespace FileIO
		{
			class File;
			enum class OpenMode;
			enum class SeekBase;
		}
		class StorageFile
		{
			std::filesystem::path path;
			void CheckFileExist(void)const;
			void PermissionException(RbsLib::Storage::FilePermission perms)const;
		public:
			StorageFile(void) noexcept;
			StorageFile(const std::string& path) noexcept;
			StorageFile(const std::filesystem::path& path) noexcept;
			StorageFile(const char* path);
			FileType::FileType GetFileType(void) const;
			std::string Path(void) const noexcept;
			bool IsExist(void)const noexcept;
			uint64_t GetFileSize(void)const;
			RbsLib::Storage::StorageFile& Append(const std::string& str)noexcept;
			RbsLib::Storage::StorageFile Parent(void)const;
			RbsLib::Storage::StorageFile operator[](const std::string& sub_path)const;
			std::string GetName(void)const;
			int Remove(void)const;
			int RemoveAll(void)const;
			bool CreateDir(const std::string& dir_name)const;
			bool CreateDirs(const std::string& dir_path)const;
			std::string GetStem(void)const;
			std::string GetExtension(void)const;
			RbsLib::Storage::FilePermission Permission(void)const noexcept;
			class Iterator
			{
			private:
				std::filesystem::directory_iterator iterator;
			public:
				Iterator(const std::filesystem::directory_iterator& iterator)noexcept;
				bool operator!=(const Iterator& other)const
				{
					return this->iterator != other.iterator;
				}
				const Iterator& operator++()
				{
					this->iterator++;
					return *this;
				}
				StorageFile operator*()const
				{
					auto it = *this->iterator;
					return StorageFile(it.path());
				}
			};
			RbsLib::Storage::StorageFile::Iterator begin() const;
			RbsLib::Storage::StorageFile::Iterator end() const;
			FileIO::File Open(RbsLib::Storage::FileIO::OpenMode open_mode,
				RbsLib::Storage::FileIO::SeekBase default_seek,
				int64_t default_offset = 0) const;
		};
	}
}