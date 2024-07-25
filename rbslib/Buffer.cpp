#include "Buffer.h"
#include <cstring>
RbsLib::BufferException::BufferException(const std::string& string)noexcept :error_reason(string) {}

const char* RbsLib::BufferException::what(void) const noexcept
{
	return error_reason.c_str();
}

RbsLib::Buffer::Buffer(uint64_t size)
	:size(size),length(0)
{
	if (size == 0)
	{
		this->data_ptr = nullptr;
	}
	else
	{
		this->data_ptr = new char[size];
	}
}

RbsLib::Buffer::Buffer(const void* data, uint64_t data_size)
	:Buffer(data_size)
{
	if (data_size == 0) return;
	this->data_ptr = new char[data_size];
	this->size = data_size;
	this->length = data_size;
	memcpy(this->data_ptr, data, data_size);
}

RbsLib::Buffer::Buffer(const Buffer& buffer)
	:Buffer(buffer.GetSize())
{
	this->Data(buffer.Data(), buffer.GetLength());
}

RbsLib::Buffer::Buffer(Buffer&& buffer) noexcept
{
	this->data_ptr = buffer.data_ptr;
	this->length = buffer.length;
	this->size = buffer.size;
	buffer.data_ptr = nullptr;
}

RbsLib::Buffer::Buffer(const std::string& str, bool zero)
{
	if (zero)
	{
		this->data_ptr = new char[str.length() + 1];
		strcpy((char*)this->data_ptr, str.c_str());
		this->length = this->size = str.length() + 1;
	}
	else
	{
		this->length = this->size = str.length();
		if (this->length == 0)
		{
			this->size = this->length = 0;
			this->data_ptr = nullptr;
			return;
		}
		this->data_ptr = new char[this->length];
		memcpy(this->data_ptr, str.c_str(), this->length);
	}
}

RbsLib::Buffer::~Buffer(void)
{
	delete[](char*)this->data_ptr;
	this->data_ptr = nullptr;
}

const RbsLib::Buffer& RbsLib::Buffer::operator=(const Buffer& buffer) noexcept
{
	this->~Buffer();
	if (buffer.size == 0)
	{
		this->data_ptr = nullptr;
		this->size = 0;
		this->length = 0;
	}
	else
	{
		this->data_ptr = new char[buffer.GetSize()];
		this->size = buffer.GetSize();
		this->length = buffer.GetLength();
		memcpy(this->data_ptr, buffer.Data(), buffer.length);
	}

	return *this;
}

const RbsLib::Buffer& RbsLib::Buffer::operator=(Buffer&& buffer) noexcept
{
	if (this == &buffer) return *this;
	this->data_ptr = buffer.data_ptr;
	this->size = buffer.GetSize();
	this->length = buffer.GetLength();
	buffer.data_ptr = nullptr;
	return *this;
}

uint8_t& RbsLib::Buffer::operator[](uint64_t index)
{
	if (index < this->GetSize()&&this->GetSize()) return *(uint8_t*)((uint8_t*)this->data_ptr + index);
	else throw RbsLib::BufferException("Buffer index out of range");
}

uint8_t RbsLib::Buffer::operator[](uint64_t index) const
{
	if (index < this->GetSize()&& this->GetSize()) return *(uint8_t*)((uint8_t*)this->data_ptr + index);
	else throw RbsLib::BufferException("Buffer index out of range");
}

const void* RbsLib::Buffer::Data(void) const noexcept
{
	return this->data_ptr;
}

uint64_t RbsLib::Buffer::GetSize(void) const noexcept
{
	return this->size;
}

uint64_t RbsLib::Buffer::GetLength(void) const noexcept
{
	return this->length;
}

std::string RbsLib::Buffer::ToString(void) const noexcept
{
	if (this->GetSize() == 0) return std::string();
	return std::string((const char*)this->Data(), this->GetLength());
}

void RbsLib::Buffer::Data(const void* data, uint64_t data_size)
{
	if (data_size > this->size) throw BufferException("data_size > buffer_size");
	else
	{
		if (!data_size)return;
		memcpy(this->data_ptr, data, data_size);
		this->length = data_size;
	}
}

void RbsLib::Buffer::SetLength(uint64_t len)
{
	if (len <= this->GetSize()) this->length = len;
}

void RbsLib::Buffer::Resize(uint64_t buffer_size)
{
	if (buffer_size == 0)
	{
		this->~Buffer();
		this->length = this->size = 0;
		this->data_ptr = nullptr;
		return;
	}
	void* data = new char[buffer_size];
	if (buffer_size >= this->length)
	{
		memcpy(data, this->data_ptr, this->length);
		delete[](char*)this->data_ptr;
		this->data_ptr = data;
		this->size = buffer_size;
	}
	else
	{
		memcpy(data, this->data_ptr, this->size);
		delete[](char*)this->data_ptr;
		this->data_ptr = data;
		this->length = this->size = buffer_size;
	}
}

void RbsLib::Buffer::PushBack(char ch)
{
	if (this->length + 1 <= this->size)
	{
		((char*)this->data_ptr)[this->length++] = ch;
	}
	else
	{
		this->Resize(this->length + 1);
		((char*)this->data_ptr)[this->length++] = ch;
	}
}

void RbsLib::Buffer::AppendToEnd(const IBuffer& buffer)
{
	std::uint64_t buffer_len = buffer.GetLength();
	if (this->length + buffer.GetLength() > this->size)
	{
		//需要重新分配空间
		this->Resize(this->size * 2 >= this->length + buffer_len ? this->size * 2 : this->length + buffer_len);
		for (std::uint64_t i = 0; i < buffer_len; ++i)
		{
			((char*)this->data_ptr)[this->length++] = ((const char*)buffer.Data())[i];
		}
	}
	for (int i = 0; i < buffer_len; ++i)
	{
		((char*)this->data_ptr)[this->length++] = ((const char*)buffer.Data())[i];
	}
}

