#ifndef DATAPACKAGE
#define DATAPACKAGE

#include "rbslib/Buffer.h"
#include "rbslib/DataType.h"
#include "rbslib/Streams.h"
#include <cstdint>
#include <exception>

class DataPackException : public std::exception {
private:
	std::string message;
public:
	DataPackException(const char* message);
	const char* what() const noexcept override;
};

class DataPack {
public:
	static RbsLib::Buffer Data(RbsLib::Streams::IInputStream& input_stream);
	static RbsLib::Buffer ReadFullData(RbsLib::Streams::IInputStream& input_stream);
	virtual auto ToBuffer() const -> RbsLib::Buffer = 0;
};

class NoCompressionDataPack : public DataPack {
public:
	RbsLib::DataType::Integer id;
	static int GetID(const RbsLib::Buffer& buffer);
};

class HandshakeDataPack : public NoCompressionDataPack {
public:
	RbsLib::DataType::Integer protocol_version;
	RbsLib::DataType::String server_address;
	std::uint16_t server_port;
	RbsLib::DataType::Integer next_state;
	void ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream);
	// 通过 NoCompressionDataPack 继承
	auto ToBuffer() const->RbsLib::Buffer override;

};

class StatusResponseDataPack : public NoCompressionDataPack {
public:
	StatusResponseDataPack();
	RbsLib::DataType::String json_response;
	// 通过 NoCompressionDataPack 继承
	auto ToBuffer() const->RbsLib::Buffer override;
};
class StatusRequestDataPack : public NoCompressionDataPack {
public:
	StatusRequestDataPack();
	void ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream);
	// 通过 NoCompressionDataPack 继承
	auto ToBuffer() const->RbsLib::Buffer override;
};

class PingDataPack : public NoCompressionDataPack {
	public:
		PingDataPack();
	std::uint64_t payload;
	void ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream);
	// 通过 NoCompressionDataPack 继承
	auto ToBuffer() const->RbsLib::Buffer override;
};

class StartLoginDataPack : public NoCompressionDataPack {
public:
	RbsLib::DataType::String user_name;
	std::uint8_t uuid[16];
	bool have_uuid = false;
	auto GetUUID() const -> std::string;
	void ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream);
	// 通过 NoCompressionDataPack 继承
	auto ToBuffer() const->RbsLib::Buffer override;
};

class LoginFailureDataPack : public NoCompressionDataPack {
	public:
	RbsLib::DataType::String reason;
	LoginFailureDataPack(const std::string& reason);
	// 通过 NoCompressionDataPack 继承
	auto ToBuffer() const->RbsLib::Buffer override;
};


#endif