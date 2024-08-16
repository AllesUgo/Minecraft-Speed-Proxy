#include "Streams.h"

RbsLib::Streams::StringStream::StringStream(const std::string& str) noexcept
{
	this->str = str;
}

RbsLib::Streams::StringStream::StringStream(const char* str)
{
	if (str == nullptr) throw StreamException("String stream can not create with nullptr");
	this->str = str;
}

RbsLib::Streams::StringStream::StringStream(void) noexcept
{
	this->str = "";
}

std::string RbsLib::Streams::StringStream::ToString() const noexcept
{
	return this->str.c_str() + this->start;
}

const RbsLib::Buffer& RbsLib::Streams::StringStream::Read(Buffer& buffer, int64_t size)
{
	if (size == 0 || size >= this->str.length() - this->start)
	{
		if (buffer.GetSize() < this->str.length() - this->start)
		{
			buffer.Data(this->str.c_str() + this->start, buffer.GetSize());
			this->start += buffer.GetSize();
		}
		else
		{
			buffer.Data(this->str.c_str() + this->start, this->str.length() - this->start);
			this->start += this->str.length() - this->start;
		}
	}
	else
	{
		if (buffer.GetSize() >= size)
		{
			buffer.Data(this->str.c_str() + this->start, size);
			this->start += size;
		}
		else
		{
			buffer.Data(this->str.c_str() + this->start, buffer.GetSize());
			this->start += buffer.GetSize();
		}
	}
	return buffer;
}

int64_t RbsLib::Streams::StringStream::Read(void* ptr, int64_t size)
{
	int64_t t = this->str.length() - this->start;
	if (size >= t)
	{

		memcpy(ptr, this->str.c_str() + start, t);
		this->start += t;
		return t;
	}
	else
	{
		memcpy(ptr, this->str.c_str() + start, size);
		this->start += size;
		return size;
	}
}

void RbsLib::Streams::StringStream::Write(const IBuffer& buffer)
{
	char* data = new char[buffer.GetLength() + 1];
	memcpy(data, buffer.Data(), buffer.GetLength());
	data[buffer.GetLength()] = 0;
	this->str += data;
	delete[]data;
}

void RbsLib::Streams::StringStream::Write(const void* ptr, int64_t size)
{
	char* data = new char[size + 1];
	memcpy(data, ptr, size);
	data[size] = 0;
	this->str += data;
	delete[]data;
}

RbsLib::Streams::StringStream::operator std::string() const noexcept
{
	return this->str.c_str() + this->start;
}

void RbsLib::Streams::StringStream::Clear(void) noexcept
{
	this->str.clear();
	this->start = 0;
}

std::string RbsLib::Streams::StringStream::ReadString(int64_t len) noexcept
{
	if (this->str.length() - this->start <= len)
	{
		int64_t t = this->start;
		this->start += this->str.length() - this->start;
		return this->str.c_str() + t;
	}
	else
	{
		char* data = new char[len + 1];
		memcpy(data, this->str.c_str() + this->start, len);
		data[len] = 0;
		std::string str = data;
		delete[] data;
		this->start += len;
		return str;
	}
}

void RbsLib::Streams::StringStream::WriteString(const std::string& str) noexcept
{
	this->str += str;
}

RbsLib::Streams::StringStream RbsLib::Streams::StringStream::operator+(const std::string& str) const noexcept
{
	return RbsLib::Streams::StringStream(this->ToString() + str);
}

const RbsLib::Streams::StringStream& RbsLib::Streams::StringStream::operator+=(const std::string& str) noexcept
{
	this->str += str;
	return *this;
}

RbsLib::Streams::StreamException::StreamException(const std::string reason) noexcept
{
	this->reason = reason;
}

const char* RbsLib::Streams::StreamException::what(void) const noexcept
{
	return this->reason.c_str();
}

RbsLib::Streams::FileInputStream::FileInputStream(const RbsLib::Storage::FileIO::File& file)
{
	if (file.CheckOpenMode(RbsLib::Storage::FileIO::OpenMode::Read))
	{
		this->file = file;
	}
	else
	{
		throw RbsLib::Streams::StreamException("File stream not readable");
	}
}

const RbsLib::Buffer& RbsLib::Streams::FileInputStream::Read(Buffer& buffer, int64_t size)
{
	if (size)
	{
		auto data = this->file.Read(buffer.GetSize());
		buffer.Data(data.Data(), data.GetLength());
	}
	else
	{
		auto data = this->file.Read(size);
		buffer.Data(data.Data(), data.GetLength());
	}
	return buffer;
}

int64_t RbsLib::Streams::FileInputStream::Read(void* ptr, int64_t size)
{
	auto data = file.Read(size);
	memcpy(ptr, data.Data(), data.GetLength());
	return data.GetLength();
}

bool RbsLib::Streams::FileInputStream::CheckEOF(void) const noexcept
{
	return file.CheckEOF();
}

RbsLib::Streams::FileOutputStream::FileOutputStream(const RbsLib::Storage::FileIO::File& file)
{
	if (file.CheckOpenMode(RbsLib::Storage::FileIO::OpenMode::Write))
	{
		this->file = file;
	}
	else
	{
		throw RbsLib::Streams::StreamException("File stream not writeable");
	}
}

void RbsLib::Streams::FileOutputStream::Write(const IBuffer& buffer)
{
	file.Write(buffer);
}

void RbsLib::Streams::FileOutputStream::Write(const void* ptr, int64_t size)
{
	if (size <= 0) return;
	file.Write(RbsLib::Buffer(ptr, size));
}

const RbsLib::Buffer& RbsLib::Streams::BufferInputStream::Read(Buffer& buffer, int64_t size)
{
	if (size > 0) {
		size = size+this->pos>this->buffer.GetSize()?this->buffer.GetSize()-this->pos:size;
	}
	if (size<=0) throw StreamException("BufferInputStream read out of range");
	buffer.Data((std::uint8_t*)this->buffer.Data()+this->pos, size);
	this->pos += size;
	return buffer;
}

int64_t RbsLib::Streams::BufferInputStream::Read(void* ptr, int64_t size)
{
	if (size > 0) {
		size = size+this->pos>this->buffer.GetSize()?this->buffer.GetSize()-this->pos:size;
	}
	if (size <= 0) return size;
	memcpy(ptr, (std::uint8_t*)this->buffer.Data()+this->pos, size);
	this->pos += size;
	return size;
}

RbsLib::Streams::BufferInputStream::BufferInputStream(const Buffer& buffer)
	:buffer(buffer)
{
}

void RbsLib::Streams::BufferInputStream::Seek(std::uint64_t pos)
{
	if (pos >= buffer.GetSize()) throw StreamException("BufferInputStream seek out of range");
}
