#include "BigInteger.h"

RbsLib::Math::BigIntegerException::BigIntegerException(const std::string& ex)
	:reason(ex)
{
}

const char* RbsLib::Math::BigIntegerException::what() const noexcept
{
	return this->reason.c_str();
}

RbsLib::Math::BigInteger::BigInteger(uint64_t bits_number)
{
	if ((bits_number & (bits_number - 1)) != 0) throw BigIntegerException("Number bits number not align to 2^n");
	this->num_len = (bits_number / 8 - 1) / sizeof(decltype(*this->num)) + 1;
	this->num = new std::uint64_t[this->num_len];
}

RbsLib::Math::BigInteger::~BigInteger()
{
	delete[]this->num;
}

RbsLib::Math::BigInteger::BigInteger(const BigInteger& b)
	:num_len(b.num_len)
{
	this->num = new std::uint64_t[this->num_len];
	std::memcpy(this->num, b.num, this->num_len * sizeof(decltype(*this->num)));
}

RbsLib::Math::BigInteger::BigInteger(BigInteger&& b)
{
	this->num = b.num;
	this->num_len = b.num_len;
	b.num = nullptr;
	b.num_len = 0;
}

auto RbsLib::Math::BigInteger::operator=(const BigInteger& b)->const BigInteger&
{
	delete[] this->num;
	this->num_len = b.num_len;
	this->num = new std::uint64_t[this->num_len];
	std::memcpy(this->num, b.num, this->num_len * sizeof(decltype(*this->num)));
	return *this;
}

auto RbsLib::Math::BigInteger::operator=(std::int64_t b)->const BigInteger&
{
	if (this->num_len < 1) throw BigIntegerException("Number is large than this object max limit");
	if (b < 0)
	{
		this->num[0] = b;
		std::memset(this->num + 1, 0xFF, (this->num_len - 1) * sizeof(decltype(*this->num)));
	}
	else
	{
		this->num[0] = b;
		std::memset(this->num + 1, 0, (this->num_len - 1) * sizeof(decltype(*this->num)));
	}
}

auto RbsLib::Math::BigInteger::operator=(BigInteger&& b)->const BigInteger&
{
	delete[] this->num;
	this->num = b.num;
	this->num_len = b.num_len;
	b.num = nullptr;
	b.num_len = 0;
	return *this;
}


