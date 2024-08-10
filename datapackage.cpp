#include "datapackage.h"
#include "rbslib/DataType.h"
#include "rbslib/Network.h"
#include <memory>



/**
 * @brief Reads data from the input stream and returns a buffer.
 * 
 * @param input_stream The input stream to read from.
 * @return RbsLib::Buffer The buffer containing the read data.
 * @throws DataPackException If there is an error parsing the size from varint or reading data from the input stream.
 */
RbsLib::Buffer DataPack::Data(RbsLib::Streams::IInputStream& input_stream)
{
    RbsLib::DataType::Integer size;
    if (!size.ParseFromVarint(input_stream)) throw DataPackException("DataPack::Data: failed to parse size from varint");
    std::unique_ptr<char[]> ptr = std::make_unique<char[]>(size.Value());
    std::int64_t need_read = size.Value();
    while (need_read > 0) {
        std::int64_t n = input_stream.Read(ptr.get()+size.Value() - need_read, need_read);
        if (n<=0) throw DataPackException("DataPack::Data: failed to read data from input_stream");
        else need_read -= n;
    }
    return RbsLib::Buffer(ptr.get(), size.Value());
}

RbsLib::Buffer DataPack::ReadFullData(RbsLib::Streams::IInputStream& input_stream)
{
	RbsLib::Buffer buffer = Data(input_stream);
	RbsLib::Buffer ret(buffer.GetLength() + 10);
	ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
	ret.AppendToEnd(buffer);
	return ret;
}



DataPackException::DataPackException(const char* message)
	: message(message)
{
}

const char* DataPackException::what() const noexcept
{
	return this->message.c_str();
}

void HandshakeDataPack::ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream)
{
	auto buffer = DataPack::Data(input_stream);
	RbsLib::Streams::BufferInputStream bis(buffer);
	this->id.ParseFromVarint(bis);
	if (this->id!=0)
		throw DataPackException("HandshakeDataPack::ParseFromInputStream: invalid id");
	this->protocol_version.ParseFromVarint(bis);
	this->server_address.ParseFromInputStream(bis);
	if (2!= bis.Read(&this->server_port, sizeof(this->server_port)))
		throw DataPackException("HandshakeDataPack::ParseFromInputStream: failed to read server_port");
	this->server_port = ntohs(this->server_port);
	this->next_state.ParseFromVarint(bis);
	if (next_state!=1 && next_state!=2)
		throw DataPackException("HandshakeDataPack::ParseFromInputStream: invalid next_state");
}

auto HandshakeDataPack::ToBuffer() const -> RbsLib::Buffer
{
	RbsLib::Buffer buffer(128);
	buffer.AppendToEnd(this->id.ToVarint());
	buffer.AppendToEnd(this->protocol_version.ToVarint());
	buffer.AppendToEnd(this->server_address.ToBuffer());
	std::uint16_t port = htons(this->server_port);
	buffer.AppendToEnd(RbsLib::Buffer(&port, sizeof(port)));
	buffer.AppendToEnd(this->next_state.ToVarint());
	RbsLib::Buffer ret(buffer.GetLength()+10);
	ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
	ret.AppendToEnd(buffer);
	return ret;
}

StatusResponseDataPack::StatusResponseDataPack()
{
	this->id = 0;
}

#include <stdio.h>
auto StatusResponseDataPack::ToBuffer() const -> RbsLib::Buffer
{
	RbsLib::Buffer buffer(128);
	buffer.AppendToEnd(this->id.ToVarint());
	buffer.AppendToEnd(this->json_response.ToBuffer());
	RbsLib::Buffer ret(buffer.GetLength() + 10);
	ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
	ret.AppendToEnd(buffer);
	return ret;
}

StatusRequestDataPack::StatusRequestDataPack()
{
	this->id = 0;
}

void StatusRequestDataPack::ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream)
{
	auto buffer = DataPack::Data(input_stream);
	RbsLib::Streams::BufferInputStream bis(buffer);
	this->id.ParseFromVarint(bis);
	if (this->id != 0)
		throw DataPackException("StatusRequestDataPack::ParseFromInputStream: invalid id");
}


/**
 * @brief 将StatusRequestDataPack对象转换为缓冲区。
 * 
 * @return RbsLib::Buffer 包含转换后数据的缓冲区。
 */
auto StatusRequestDataPack::ToBuffer() const -> RbsLib::Buffer
{
    RbsLib::Buffer buffer(16);
    buffer.AppendToEnd(this->id.ToVarint());
    RbsLib::Buffer ret(buffer.GetLength() + 10);
    ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
    ret.AppendToEnd(buffer);
    return ret;
}

PingDataPack::PingDataPack()
{
	this->id = 1;
}

void PingDataPack::ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream)
{
	auto buffer = DataPack::Data(input_stream);
	RbsLib::Streams::BufferInputStream bis(buffer);
	this->id.ParseFromVarint(bis);
	if (this->id != 1)
		throw DataPackException("PingDataPack::ParseFromInputStream: invalid id");
	if (8 != bis.Read(&this->payload, sizeof(this->payload)))
		throw DataPackException("PingDataPack::ParseFromInputStream: failed to read payload");
}

auto PingDataPack::ToBuffer() const -> RbsLib::Buffer
{
	RbsLib::Buffer buffer(32);
	buffer.AppendToEnd(this->id.ToVarint());
	buffer.AppendToEnd(RbsLib::Buffer(&payload, sizeof(payload)));
	RbsLib::Buffer ret(buffer.GetLength() + 10);
	ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
	ret.AppendToEnd(buffer);
	return ret;
}

auto StartLoginDataPack::GetUUID() const -> std::string
{
	char buffer[64];
	sprintf(buffer, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		this->uuid[0], this->uuid[1], this->uuid[2], this->uuid[3],
		this->uuid[4], this->uuid[5],
		this->uuid[6], this->uuid[7],
		this->uuid[8], this->uuid[9],
		this->uuid[10], this->uuid[11], this->uuid[12], this->uuid[13], this->uuid[14], this->uuid[15]);
	return std::string(buffer);
}

void StartLoginDataPack::ParseFromInputStream(RbsLib::Streams::IInputStream& input_stream)
{
	auto buffer = DataPack::Data(input_stream);
	RbsLib::Streams::BufferInputStream bis(buffer);
	this->id.ParseFromVarint(bis);
	if (this->id!=0)
		throw DataPackException("StartLoginDataPack::ParseFromInputStream: invalid id");
	this->user_name.ParseFromInputStream(bis);
	if (16 != bis.Read(this->uuid, 16))
		this->have_uuid = false;
	else this->have_uuid = true;
}

auto StartLoginDataPack::ToBuffer() const -> RbsLib::Buffer
{
	RbsLib::Buffer buffer(this->user_name.length()+20);
	buffer.AppendToEnd(this->id.ToVarint());
	buffer.AppendToEnd(this->user_name.ToBuffer());
	if (this->have_uuid)
		buffer.AppendToEnd(RbsLib::Buffer(this->uuid, 16));
	RbsLib::Buffer ret(buffer.GetLength() + 10);
	ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
	ret.AppendToEnd(buffer);
	return ret;
}

int NoCompressionDataPack::GetID(const RbsLib::Buffer& buffer)
{
	RbsLib::Streams::BufferInputStream bis(buffer);
	RbsLib::DataType::Integer id;
	id.ParseFromVarint(bis);
	if (id.Value()!=buffer.GetLength() - id.ToVarint().GetLength()) throw DataPackException("NoCompressionDataPack::GetID: invalid size");
	id.ParseFromVarint(bis);
	return id.Value();
}

LoginFailureDataPack::LoginFailureDataPack(const std::string& reason)
{
	this->id = 0;
	auto str = std::string("\"")+reason+std::string("\"");
	this->reason += RbsLib::DataType::String(str);
}

auto LoginFailureDataPack::ToBuffer() const -> RbsLib::Buffer
{
	RbsLib::Buffer buffer(this->reason.length()+20);
	buffer.AppendToEnd(this->id.ToVarint());
	buffer.AppendToEnd(this->reason.ToBuffer());
	RbsLib::Buffer ret(buffer.GetLength() + 10);
	ret.AppendToEnd(RbsLib::DataType::Integer(buffer.GetLength()).ToVarint());
	ret.AppendToEnd(buffer);
	return ret;
}
