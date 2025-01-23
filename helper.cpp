#include "helper.h"
#include "datapackage.h"
#include "rbslib/Network.h"
#include <stdexcept>
auto Helper::GetRemoteServerMotd(const std::string& remote_server_addr, std::uint16_t remote_server_port, bool is_ipv6) -> neb::CJsonObject
{
	auto remote_server = is_ipv6 ? RbsLib::Network::TCP::TCPClient::Connect6(remote_server_addr, remote_server_port) : RbsLib::Network::TCP::TCPClient::Connect(remote_server_addr, remote_server_port);
	int flag = 1;
	remote_server.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
	HandshakeDataPack handshake_data_pack;
	handshake_data_pack.id = 0;
	handshake_data_pack.server_address = RbsLib::DataType::String(remote_server_addr);
	handshake_data_pack.server_port = remote_server_port;
	handshake_data_pack.next_state = 1;
	handshake_data_pack.protocol_version = 754;
	remote_server.Send(handshake_data_pack.ToBuffer());
	RbsLib::Network::TCP::TCPStream stream(remote_server);
	//发送状态请求
	StatusRequestDataPack status_request_data_pack;
	remote_server.Send(status_request_data_pack.ToBuffer());
	//接收状态并丢弃
	StatusResponseDataPack status_response_data_pack;
	status_response_data_pack.ParseFromInputStream(stream);
	neb::CJsonObject json;
	if (!json.Parse(status_response_data_pack.json_response)) throw std::runtime_error("Invalid motd json: "+json.GetErrMsg());
	return json;
}
