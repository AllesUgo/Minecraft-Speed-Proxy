#pragma once
//һЩ���������������û�ʹ��

#include <string>

#include "json/CJsonObject.h"

namespace Helper 
{
	auto GetRemoteServerMotd(const std::string& remote_server_addr, std::uint16_t remote_server_port, bool is_ipv6) -> neb::CJsonObject;
}