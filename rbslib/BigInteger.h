#pragma once
#include <cstdint>
#include <string>
#include <cstring>
#include <exception>
namespace RbsLib::Math
{
	class BigIntegerException:std::exception
	{
	private:
		std::string reason;
	public:
		BigIntegerException(const std::string& ex);
		const char* what() const noexcept override;
	};
	class BigInteger 
	{
	private:
		std::uint64_t* num=nullptr;
		uint64_t num_len=0;
	public:
		BigInteger() = default;
		BigInteger(uint64_t bits_number);//align to 2^n
		~BigInteger();
		BigInteger(const BigInteger& b);
		BigInteger(BigInteger&& b);
		const BigInteger& operator=(const BigInteger& b);
		const BigInteger& operator=(std::int64_t b);
		const BigInteger& operator=(BigInteger&& b);
		std::string ToString() const;
	};
}