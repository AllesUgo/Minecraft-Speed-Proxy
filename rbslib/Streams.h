#pragma once
#include "BaseType.h"
#include "Buffer.h"
#include "FileIO.h"
#include <cstdint>
#include <exception>
#include <string>
#include "../asio/import_asio.h"
namespace RbsLib
{
	namespace Streams
	{
		namespace StreamMode
		{
			enum StreamMode
			{
				ReadOnly,
				WriteOnly,
				ReadAndWrite
			};
		}
		class StreamException :public std::exception
		{
		private:
			std::string reason;
		public:
			StreamException(const std::string reason) noexcept;
			const char* what(void)const noexcept override;
		};
		class IInputStream
		{
		public:
			virtual const Buffer& Read(Buffer& buffer, int64_t size = 0) = 0;
			virtual int64_t Read(void* ptr, int64_t size) = 0;
		};
		class IOutputStream
		{
		public:
			virtual void Write(const IBuffer& buffer) = 0;
			virtual void Write(const void* ptr, int64_t size) = 0;
		};
		class IAsyncInputStream
		{
		public:
			virtual asio::awaitable<const Buffer&> ReadAsync(Buffer& buffer, int64_t size = 0) = 0;
			virtual asio::awaitable<int64_t> ReadAsync(void* ptr, int64_t size) = 0;
		};
		class IAsyncOutputStream
		{
			public:
			virtual asio::awaitable<void> WriteAsync(const IBuffer& buffer) = 0;
			virtual asio::awaitable<void> WriteAsync(const void* ptr, int64_t size) = 0;
		};
		class IOStream :public IInputStream, public IOutputStream
		{};
		class IStream :public IInputStream, public IOutputStream
		{
		private:
			StreamMode::StreamMode stream_mode;
		public:
		};
		class StringStream :public IInputStream, public IOutputStream
		{
		private:
			std::string str;
			int64_t start = 0;
		public:
			StringStream(const std::string& str)noexcept;
			StringStream(const char* str);
			StringStream(void) noexcept;
			std::string ToString()const noexcept;
			const Buffer& Read(Buffer& buffer, int64_t size = 0) override;
			int64_t Read(void* ptr, int64_t size) override;
			void Write(const IBuffer& buffer) override;
			void Write(const void* ptr, int64_t size)override;
			operator std::string()const noexcept;
			void Clear(void) noexcept;
			std::string ReadString(int64_t len) noexcept;
			void WriteString(const std::string& str) noexcept;
			RbsLib::Streams::StringStream operator+(const std::string& str) const noexcept;
			const RbsLib::Streams::StringStream& operator+=(const std::string& str) noexcept;
		};
		class FileInputStream :public IInputStream
		{
		private:
			RbsLib::Storage::FileIO::File file;
		public:
			FileInputStream(const RbsLib::Storage::FileIO::File& file);
			const Buffer& Read(Buffer& buffer, int64_t size = 0)override;
			int64_t Read(void* ptr, int64_t size) override;
			bool CheckEOF(void) const noexcept;
		};
		class FileOutputStream :public IOutputStream
		{
		private:
			RbsLib::Storage::FileIO::File file;
		public:
			FileOutputStream(const RbsLib::Storage::FileIO::File& file);
			void Write(const IBuffer& buffer) override;
			void Write(const void* ptr, int64_t size) override;
		};
		class BufferInputStream :public IInputStream {
		private:
			const Buffer& buffer;
			std::uint64_t pos = 0;
		public:

			// 通过 IInputStream 继承
			const Buffer& Read(Buffer& buffer, int64_t size) override;
			int64_t Read(void* ptr, int64_t size) override;
			
			BufferInputStream(const Buffer& buffer);
			void Seek(std::uint64_t pos);

			std::uint64_t RemainLength() const;
		};
	}
}
