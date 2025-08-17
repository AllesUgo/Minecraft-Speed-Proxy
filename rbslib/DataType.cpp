#include "DataType.h"
#include "memory"
RbsLib::DataType::DataTypeException::DataTypeException(const char* message)
	: message(message)
{
}

const char* RbsLib::DataType::DataTypeException::what() const noexcept
{
	return message.c_str();
}

RbsLib::DataType::Integer::Integer(std::int64_t value)
	: value(value)
{
}

std::int64_t RbsLib::DataType::Integer::Value() const
{
	return this->value;
}

void RbsLib::DataType::Integer::Value(std::int64_t value)
{
	this->value = value;
}

auto RbsLib::DataType::Integer::operator+(const Integer& y) const -> Integer
{
	return this->Value() + y.Value();
}

auto RbsLib::DataType::Integer::operator-(const Integer& y) const -> Integer
{
	return this->Value() - y.Value();
}

auto RbsLib::DataType::Integer::operator*(const Integer& y) const -> Integer
{
	return this->Value() * y.Value();
}

auto RbsLib::DataType::Integer::operator/(const Integer& y) const -> Integer
{
	if (y.Value() == 0)
	{
		throw DataTypeException("Division by zero");
	}
	return Integer(this->Value() / y.Value());
}

auto RbsLib::DataType::Integer::operator%(const Integer& y) const -> Integer
{
	if (y.Value() == 0)
	{
		throw DataTypeException("Modulo by zero");
	}
	return Integer(this->Value() % y.Value());
}

RbsLib::DataType::Integer& RbsLib::DataType::Integer::operator+=(const Integer& y)
{
	this->value += y.Value();
	return *this;
}

RbsLib::DataType::Integer& RbsLib::DataType::Integer::operator-=(const Integer& y)
{
	this->value -= y.Value();
	return *this;
}

RbsLib::DataType::Integer& RbsLib::DataType::Integer::operator*=(const Integer& y)
{
	this->value *= y.Value();
	return *this;
}

RbsLib::DataType::Integer& RbsLib::DataType::Integer::operator/=(const Integer& y)
{
	if (y.Value() == 0)
	{
		throw DataTypeException("Division by zero");
	}
	this->value /= y.Value();
	return *this;
}

RbsLib::DataType::Integer& RbsLib::DataType::Integer::operator%=(const Integer& y)
{
	if (y.Value() == 0)
	{
		throw DataTypeException("Modulo by zero");
	}
	this->value %= y.Value();
	return *this;
}

auto RbsLib::DataType::Integer::ToString() const -> std::string
{
	return std::to_string(this->Value());
}

auto RbsLib::DataType::Integer::ToVarint() const -> Buffer
{
	if (this->Value() == 0)
	{
		Buffer buffer(1);
		buffer.SetLength(1);
		buffer[0] = 0;
		return buffer;
	}
	if (this->Value() <0)
	{
		throw DataTypeException("Varint cannot be negative");
	}
	Buffer buffer;
	std::int64_t value = this->Value();
	while (value > 0)
	{
		std::uint8_t byte = value & 0x7F;
		value >>= 7;
		if (value > 0)
		{
			byte |= 0x80;
		}
		buffer.PushBack(byte);
	}
	return buffer;
}

/**
 * @brief 从Varint格式的输入流中解析整数值。
 * 
 * @param is 输入流对象。
 * @return 解析是否成功。
 */
bool RbsLib::DataType::Integer::ParseFromVarint(RbsLib::Streams::IInputStream& is)
{
	std::uint8_t byte;
	int n = 0;
	std::uint64_t temp = 0;
	for (int i = 0; i < 10; i++)
	{
		int recved = is.Read(&byte, 1);
		if (1 != recved) break;
		temp |= ((std::uint64_t)(byte & 0x7F) << (n++ * 7));
		if ((byte & 0x80) == 0)
		{
			this->Value(temp);
			return true;
		}
	}
	return false;
}

asio::awaitable<bool> RbsLib::DataType::Integer::ParseFromVarint(RbsLib::Streams::IAsyncInputStream& is)
{
	std::uint8_t byte;
	int n = 0;
	std::uint64_t temp = 0;
	for (int i = 0; i < 10; i++)
	{
		int recved = co_await is.ReadAsync(&byte, 1);
		if (1 != recved) break;
		temp |= ((std::uint64_t)(byte & 0x7F) << (n++ * 7));
		if ((byte & 0x80) == 0)
		{
			this->Value(temp);
			co_return true;
		}
	}
	co_return false;
}

void RbsLib::DataType::String::ParseFromInputStream(RbsLib::Streams::IInputStream& is)
{
	std::uint8_t byte;
	std::int64_t size;
	RbsLib::DataType::Integer size_data;
	if (!size_data.ParseFromVarint(is))
	{
		throw DataTypeException("String::ParseFromInputStream: failed to parse size from varint");
	}
	size = size_data.Value();
	std::unique_ptr<char[]> ptr = std::make_unique<char[]>(size);
	std::int64_t need_read = size;
	while (need_read > 0)
	{
		std::int64_t n = is.Read(ptr.get(), need_read);
		if (n <= 0)
		{
			throw DataTypeException("String::ParseFromInputStream: failed to read data from input_stream");
		}
		else
		{
			need_read -= n;
		}
	}
	this->assign(ptr.get(), size);
}

asio::awaitable<bool> RbsLib::DataType::String::ParseFromInputStream(RbsLib::Streams::IAsyncInputStream& is)
{
	std::uint8_t byte;
	std::int64_t size;
	RbsLib::DataType::Integer size_data;
	if (!co_await size_data.ParseFromVarint(is))
	{
		throw DataTypeException("String::ParseFromInputStream: failed to parse size from varint");
	}
	size = size_data.Value();
	std::unique_ptr<char[]> ptr = std::make_unique<char[]>(size);
	std::int64_t need_read = size;
	while (need_read > 0)
	{
		std::int64_t n = co_await is.ReadAsync(ptr.get(), need_read);
		if (n <= 0)
		{
			throw DataTypeException("String::ParseFromInputStream: failed to read data from input_stream");
		}
		else
		{
			need_read -= n;
		}
	}
	this->assign(ptr.get(), size);
}

auto RbsLib::DataType::String::ToBuffer(void) const -> RbsLib::Buffer
{
	RbsLib::Buffer buffer(this->size()+10);
	buffer.AppendToEnd(RbsLib::DataType::Integer(this->size()).ToVarint());
	buffer.AppendToEnd(RbsLib::Buffer(this->data(),this->size()));
	return buffer;
}
