#include "netstat.hpp"
#include "netstat-helpers.hpp"
#if BOOST_OS_WINDOWS

#include <ext/errors.hpp>
#include <ext/log/logging_macros.hpp>

#include <boost/scope_exit.hpp>
#include <fmt/format.h>

#include <ext/net/socket_fwd.hpp>
#include <ext/net/socket_base.hpp>
#include <ext/net/socket_include.hpp>

#include <winsock2.h>
#include <tcpmib.h>
#include <udpmib.h>
#include <iphlpapi.h>
#include <windows.h>

static std::string state_str(DWORD state)
{
	switch (state)
	{
		case MIB_TCP_STATE_CLOSED:      return "CLOSED";
		case MIB_TCP_STATE_LISTEN:      return "LISTEN";
		case MIB_TCP_STATE_SYN_SENT:    return "SYN-SENT";
		case MIB_TCP_STATE_SYN_RCVD:    return "SYN-RECEIVED";
		case MIB_TCP_STATE_ESTAB:       return "ESTABLISHED";
		case MIB_TCP_STATE_FIN_WAIT1:   return "FIN-WAIT-1";
		case MIB_TCP_STATE_FIN_WAIT2:   return "FIN-WAIT-2";
		case MIB_TCP_STATE_CLOSE_WAIT:  return "CLOSE-WAIT";
		case MIB_TCP_STATE_CLOSING:     return "CLOSING";
		case MIB_TCP_STATE_LAST_ACK:    return "LAST-ACK";
		case MIB_TCP_STATE_TIME_WAIT:   return "TIME-WAIT";
		case MIB_TCP_STATE_DELETE_TCB:  return "DELETE-TCB";
		default:                        return fmt::format("UNKNOWN state value {}", state);
	}
}

static std::unique_ptr<MIB_TCPTABLE2> obtain_tcptable()
{
	std::unique_ptr<MIB_TCPTABLE2> tcpTable;
	ULONG ret, size = 0;
	
	for (;;)
	{
		ret = GetTcpTable2(tcpTable.get(), &size, false);
		if (ret == NO_ERROR) break;
		
		if (ret == ERROR_INSUFFICIENT_BUFFER)
		{
			//auto count = sizeof(MIB_TCPROW2) * size + sizeof(DWORD);
			tcpTable.reset(static_cast<PMIB_TCPTABLE2>(operator new(size)));
			//tcpTable->dwNumEntries = size;
			continue;
		}
		
		ext::throw_last_system_error("GetTcpTable2 failed");
	};
	
	return tcpTable;
}

static std::unique_ptr<MIB_TCP6TABLE2> obtain_tcp6table()
{
	std::unique_ptr<MIB_TCP6TABLE2> tcpTable;
	ULONG ret, size = 0;
	
	for (;;)
	{
		ret = GetTcp6Table2(tcpTable.get(), &size, false);
		if (ret == NO_ERROR) break;
		
		if (ret == ERROR_INSUFFICIENT_BUFFER)
		{
			//auto count = sizeof(MIB_TCP6ROW2) * size + sizeof(DWORD);
			tcpTable.reset(static_cast<PMIB_TCP6TABLE2>(operator new(size)));
			//tcpTable->dwNumEntries = size;
			continue;
		}
		
		ext::throw_last_system_error("GetTcp6Table2 failed");
	};
	
	return tcpTable;
}

unsigned int find_counter_socket_pid(int sock, ext::log::logger * logger)
{
	return find_counter_socket(find_socket_pid, "find_counter_socket_pid", sock, logger);
}

unsigned int find_socket_pid(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger)
{
	assert(sock_addr);
	assert(peer_addr);
	
	if (proto != IPPROTO_TCP)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: unsupported socket protocol = {}", proto_str(proto)));
	
	if (sock_addr->sa_family != peer_addr->sa_family)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: sock_addr->sa_family <> peer_addr->sa_family({} <> {})", sock_addr->sa_family, peer_addr->sa_family));
	
	if (sock_addr->sa_family != AF_INET and sock_addr->sa_family != AF_INET6)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: bad sock_addr->sa_family = {}, only AF_INET or AF_INET6 supported", sock_addr->sa_family));
	
	EXTLOG_DEBUG_FMT(logger, "find_socket_pid: searching pid for proto = {}, {} <- {}",
	                 proto_str(proto), ext::net::sockaddr_endpoint_noexcept(sock_addr), ext::net::sockaddr_endpoint_noexcept(peer_addr));
	
	DWORD pid = 0;
	
	if (sock_addr->sa_family == AF_INET)
	{
		auto tcpTable = obtain_tcptable();
		auto * first = tcpTable->table;
		auto * last  = first + tcpTable->dwNumEntries;
		
		auto * sock_addr_in = reinterpret_cast<const sockaddr_in *>(sock_addr),
		     * peer_addr_in = reinterpret_cast<const sockaddr_in *>(peer_addr);
		
		for (; first != last; ++first)
		{
			auto & row = *first;
			short unsigned local_port  = row.dwLocalPort  & 0xFFFFu;
			short unsigned remote_port = row.dwRemotePort & 0xFFFFu;
			
			struct in_addr local_inaddr, remote_inaddr;
			local_inaddr.s_addr = row.dwLocalAddr;
			remote_inaddr.s_addr = row.dwLocalAddr;
			
			bool matched = sock_addr_in->sin_addr.s_addr ==  local_inaddr.s_addr and sock_addr_in->sin_port == local_port
			           and peer_addr_in->sin_addr.s_addr == remote_inaddr.s_addr and peer_addr_in->sin_port == remote_port;
			
			if (matched)
			{
				pid = row.dwOwningPid;
				break;
			}
		}	
	}
	else if (sock_addr->sa_family == AF_INET6)
	{
		auto tcpTable = obtain_tcp6table();
		auto * first = tcpTable->table;
		auto * last  = first + tcpTable->dwNumEntries;
		
		auto * sock_addr_in = reinterpret_cast<const sockaddr_in6 *>(sock_addr),
		     * peer_addr_in = reinterpret_cast<const sockaddr_in6 *>(peer_addr);
		
		for (; first != last; ++first)
		{
			auto & row = *first;
			short unsigned local_port  = row.dwLocalPort  & 0xFFFFu;
			short unsigned remote_port = row.dwRemotePort & 0xFFFFu;
			
			const in6_addr & local_addr = row.LocalAddr;
			const in6_addr & remote_addr = row.RemoteAddr;
			
			constexpr unsigned ipv6_size = sizeof(in6_addr::s6_addr);
			static_assert (ipv6_size == 16);
			
			bool matched = std::memcmp(sock_addr_in->sin6_addr.s6_addr, local_addr.s6_addr, ipv6_size) == 0 and sock_addr_in->sin6_port == local_port
			           and std::memcmp(peer_addr_in->sin6_addr.s6_addr, remote_addr.s6_addr, ipv6_size) == 0 and peer_addr_in->sin6_port == remote_port;
			
			if (matched)
			{
				pid = row.dwOwningPid;
				break;
			}
		}	
	}
	
	
	EXTLOG_DEBUG_FMT(logger, "find_socket_pid: searching pid result: pid = {}", pid);
	return pid;
}

#endif // BOOST_OS_WINDOWS
