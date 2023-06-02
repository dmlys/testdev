#include "netstat-helpers.hpp"

std::string proto_str(int proto)
{
	if (proto == IPPROTO_TCP) return fmt::format("IPPROTO_TCP({})", proto);
	if (proto == IPPROTO_UDP) return fmt::format("IPPROTO_UDP({})", proto);
	
	return fmt::format("IPPROTO({})", proto);
}
