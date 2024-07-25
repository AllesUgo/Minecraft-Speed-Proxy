#ifndef DATATYPE_H
#define DATATYPE_H
#include "BaseType.h"
#include "Buffer.h"
#include "Streams.h"
#include <cstdint>
#include <exception>

namespace RbsLib::DataType {
	class DataTypeException : public std::exception {
	private:
		std::string message;
	public:
		DataTypeException(const char* message);
		const char* what() const noexcept override;
	};

	class Integer {
	protected:
		std::int64_t value;
	public:
		Integer(std::int64_t value = 0);
		std::int64_t Value() const;
		void Value(std::int64_t value);
		auto operator<=> (const Integer&) const = default;
		auto operator+(const Integer&y) const -> Integer;
		auto operator-(const Integer&y) const -> Integer;
		auto operator*(const Integer&y) const -> Integer;
		auto operator/(const Integer&y) const -> Integer;
		auto operator%(const Integer&y) const -> Integer;
		auto operator+=(const Integer&y) -> Integer&;
		auto operator-=(const Integer&y) -> Integer&;
		auto operator*=(const Integer&y) -> Integer&;
		auto operator/=(const Integer&y) -> Integer&;
		auto operator%=(const Integer&y) -> Integer&;
		auto ToString() const -> std::string;
		auto ToVarint() const -> Buffer;
		bool ParseFromVarint(RbsLib::Streams::IInputStream& is);
	};

	class String :public std::string {
	public:
		void ParseFromInputStream(RbsLib::Streams::IInputStream& is);
		auto ToBuffer(void) const->RbsLib::Buffer;
	};
}
#endif // !DATATYPE_H
