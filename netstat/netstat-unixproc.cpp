#include "netstat.hpp"
#include "netstat-helpers.hpp"
#if BOOST_OS_UNIX

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <tuple>
#include <istream>
#include <fstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <dirent.h>

#include <boost/scope_exit.hpp>
#include <fmt/format.h>

#include <ext/errors.hpp>
#include <ext/log/logging_macros.hpp>
#include <ext/net/socket_base.hpp>


static auto read_line(const char * fname, std::istream & is, char * buffer, std::size_t buffer_size)
{
	is.getline(buffer, buffer_size);
	if (is or is.eof()) return is.gcount();
	
	if (static_cast<std::size_t>(is.gcount()) >= buffer_size - 1)
	{
		auto errmsg = fmt::format("find_socket_inode: failed to read line from {}, line is to long(gz{} >= bz{})", fname, is.gcount(), buffer_size);
		throw std::runtime_error(errmsg);
	}
	else
	{
		ext::throw_last_errno("find_socket_inode: failed to read line from {}", fname);
	}
}

static void parse_addr(in6_addr * addr, const char buffer[32])
{
	// see linux kernel ipv6/tcp_ipv6.c get_tcp6_sock
	// linux kernel prints ipv6 addr as follows 
	// seq_printf(seq, "... %08X%08X%08X%08X:%04X ...", ..., src->s6_addr32[0], src->s6_addr32[1], src->s6_addr32[2], src->s6_addr32[3], srcp, ...)
	// basicly breaking 16bytes into array of uint32_t (se also in6_addr::s6_addr32)
	// each one is printed as hex number
	
	auto u0 = reinterpret_cast<std::uint32_t *>(addr->s6_addr + 0 * sizeof(std::uint32_t));
	auto u1 = reinterpret_cast<std::uint32_t *>(addr->s6_addr + 1 * sizeof(std::uint32_t));
	auto u2 = reinterpret_cast<std::uint32_t *>(addr->s6_addr + 2 * sizeof(std::uint32_t));
	auto u3 = reinterpret_cast<std::uint32_t *>(addr->s6_addr + 3 * sizeof(std::uint32_t));
	
	sscanf(buffer, "%8X%8X%8X%8X", u0, u1, u2, u3);
}

auto find_socket_inode_procfs(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger) -> std::tuple<uid_t, ino64_t>
{
	assert(sock_addr);
	assert(peer_addr);
	
	if (proto != IPPROTO_TCP and proto != IPPROTO_UDP)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: unsupported socket protocol = {}", proto_str(proto)));
	
	if (sock_addr->sa_family != peer_addr->sa_family)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: sock_addr->sa_family <> peer_addr->sa_family({} <> {})", sock_addr->sa_family, peer_addr->sa_family));
	
	if (sock_addr->sa_family != AF_INET and sock_addr->sa_family != AF_INET6)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: bad sock_addr->sa_family = {}, only AF_INET or AF_INET6 supported", sock_addr->sa_family));
	
	
	EXTLOG_DEBUG_FMT(logger, "find_socket_inode_procfs: searching inode for proto = {}, {} <- {}",
	                 proto_str(proto), ext::net::sockaddr_endpoint_noexcept(sock_addr), ext::net::sockaddr_endpoint_noexcept(peer_addr));
		
	constexpr unsigned buffer_size = 1024;
	std::string buffer;
	buffer.resize(buffer_size);
	
	std::tuple<uid_t, ino64_t> result = {0, 0};
	
	if (sock_addr->sa_family == AF_INET)
	{
		auto * sock_addr_in = reinterpret_cast<const sockaddr_in *>(sock_addr),
		     * peer_addr_in = reinterpret_cast<const sockaddr_in *>(peer_addr);
		
		auto sock_port = ntohs(sock_addr_in->sin_port);
		auto peer_port = ntohs(peer_addr_in->sin_port);
		
		auto fname = proto == IPPROTO_TCP ? "/proc/net/tcp" : "/proc/net/udp";
		std::ifstream ifs(fname);
		if (not ifs)
			ext::throw_last_errno("find_socket_inode_procfs: failed to open {}", fname);
		
		// first line are headers - ignore for now
		read_line(fname, ifs, buffer.data(), buffer_size);
	
		while (ifs)
		{
			auto read = read_line(fname, ifs, buffer.data(), buffer_size);
			
			in_addr local_inaddr, remote_inaddr;
			unsigned short local_port, remote_port;
			uid_t uid;
			ino64_t inode;
			
			// sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode 
			auto nread = sscanf(buffer.c_str(), "%*d: %x:%hx %x:%hx %*x %*x:%*x %*x:%*x %*x %d %*d %jd", &local_inaddr.s_addr, &local_port, &remote_inaddr.s_addr, &remote_port, &uid, &inode);
			if (nread < 6)
			{
				EXTLOG_DEBUG_FMT(logger, "find_socket_inode_procfs: failed to parse line from {}, line content: {}", fname, std::string_view(buffer.data(), read));
				throw std::runtime_error("find_socket_inode_procfs: failed to parse line");
			}
			
			bool matched = sock_addr_in->sin_addr.s_addr == local_inaddr.s_addr  and sock_port == local_port
			           and peer_addr_in->sin_addr.s_addr == remote_inaddr.s_addr and peer_port == remote_port;
			
			if (matched)
			{
				result = std::make_tuple(uid, inode);
				break;
			}
		}
	}
	else if (sock_addr->sa_family == AF_INET6)
	{
		auto * sock_addr_in = reinterpret_cast<const sockaddr_in6 *>(sock_addr), 
		     * peer_addr_in = reinterpret_cast<const sockaddr_in6 *>(peer_addr);
		
		auto sock_port = ntohs(sock_addr_in->sin6_port);
		auto peer_port = ntohs(peer_addr_in->sin6_port);
		
		auto fname = proto == IPPROTO_TCP ? "/proc/net/tcp6" : "/proc/net/udp6";
		std::ifstream ifs(fname);
		if (not ifs)
			ext::throw_last_errno("find_socket_inode_procfs: failed to open {}", fname);
		
		// first line are headers - ignore for now
		read_line(fname, ifs, buffer.data(), buffer_size);
		
		while (ifs)
		{
			auto read = read_line(fname, ifs, buffer.data(), buffer_size);
			
			constexpr unsigned ipv6_size = sizeof(in6_addr::s6_addr);
			static_assert (ipv6_size == 16);
			
			// ipv6 addr is 128bit or 16 bytes long
			// /proc/net/tcp6 provides them hex encoded, so we need to read 32 chars for each ipv6 addr
			char local_addr_buffer[ipv6_size * 2], remote_addr_buffer[ipv6_size * 2];
			in6_addr local_inaddr, remote_inaddr;
			unsigned short local_port, remote_port;
			uid_t uid;
			ino64_t inode;
			
			errno = 0;
			// sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode 
			auto nread = sscanf(buffer.c_str(), "%*d: %32c:%hx %32c:%hx %*x %*x:%*x %*x:%*x %*x %d %*d %jd", local_addr_buffer, &local_port, remote_addr_buffer, &remote_port, &uid, &inode);
			if (nread < 6)
			{
				EXTLOG_DEBUG_FMT(logger, "find_socket_inode_procfs: failed to parse line from {}, line content: {}", fname, std::string_view(buffer.data(), read));
				throw std::runtime_error("find_socket_inode_procfs: failed to parse line");
			}
			
			parse_addr(&local_inaddr, local_addr_buffer);
			parse_addr(&remote_inaddr, local_addr_buffer);
			
			bool matched = std::memcmp(sock_addr_in->sin6_addr.s6_addr, local_inaddr.s6_addr,  ipv6_size) == 0 and sock_port == local_port
			           and std::memcmp(peer_addr_in->sin6_addr.s6_addr, remote_inaddr.s6_addr, ipv6_size) == 0 and peer_port == remote_port;
			
			if (matched)
			{				
				result = std::make_tuple(uid, inode);
				break;
			}
		}
	}
	
	EXTLOG_DEBUG_FMT(logger, "find_socket_inode_procfs: searching inode result: uid = {}, inode = {}", std::get<0>(result), std::get<1>(result));
	return result;
}

static dirent * readdir_helper(DIR * dir, const char * path)
{
	errno = 0;
	dirent * entry = readdir(dir);
	if (entry) return entry;
	
	if (errno == 0) return nullptr; // eof
	
	ext::throw_last_errno("find_pid_by_inode: readdir for \"{}\" failed", path);
}

pid_t find_pid_by_inode(ino64_t inode, ext::log::logger * logger)
{
	if (inode <= 0)
		return 0;
	
	EXTLOG_DEBUG_FMT(logger, "find_pid_by_inode: searching pid for inode = {}", inode);
	
	std::string needle = fmt::format("socket:[{}]", inode);
	auto isdigit = [](char ch) { return ch >= '0' and ch <= '9'; };
	
	std::string linkpath;
	linkpath.reserve(1024);
	
	DIR * procdir = nullptr, * fddir = nullptr;
	BOOST_SCOPE_EXIT_ALL(&procdir, &fddir)
	{
		if (fddir) closedir(fddir);
		if (procdir) closedir(procdir);
	};
	
	procdir = opendir("/proc");
	if (not procdir)
		ext::throw_last_errno("find_pid_by_inode: failed to open /proc dir for reading");
	
	EXTLOG_TRACE_STR(logger, "find_pid_by_inode: processing /proc directory");
	
	for (;;)
	{
		dirent * entry = readdir_helper(procdir, "/proc");
		if (not entry) break;
		
		if (entry->d_type != DT_DIR)
			continue;
		
		auto * pid_first = entry->d_name;
		auto * pid_last  = pid_first + strnlen(pid_first, NAME_MAX);
		
		if (not std::all_of(pid_first, pid_last, isdigit))
			continue;
		
		linkpath.clear();
		linkpath.append("/proc/");
		linkpath.append(pid_first, pid_last);
		linkpath.append("/fd");
		auto path_pos = linkpath.size();
		
		fddir = opendir(linkpath.c_str());
		if (not fddir)
		{
			EXTLOG_TRACE_FMT(logger, "find_pid_by_inode: failed to open {}, skipping", linkpath);
			continue;
		}
		
		EXTLOG_TRACE_FMT(logger, "find_pid_by_inode: processing {}", linkpath);
		for (;;)
		{
			dirent * entry = readdir_helper(fddir, linkpath.c_str());
			if (not entry) break;
			
			if (entry->d_type != DT_LNK)
				continue;
			
			auto * fd_first = entry->d_name;
			auto * fd_last  = fd_first + strnlen(fd_first, NAME_MAX);
			
			if (not std::all_of(fd_first, fd_last, isdigit))
				continue;
			
			linkpath.erase(path_pos);
			linkpath += '/';
			linkpath.append(fd_first, fd_last);
			
			char buffer[NAME_MAX];
			auto read = ::readlink(linkpath.c_str(), buffer, NAME_MAX);
			if (read < 0)
			{
				EXTLOG_DEBUG_FMT(logger, "find_pid_by_inode: failed to readlink {}, continue", linkpath);
				continue;
			}
			else
			{
				std::string_view resolved_link(buffer, read);
				if (resolved_link == needle)
				{
					EXTLOG_DEBUG_FMT(logger, "find_pid_by_inode: {} points to {}, returning pid = {}", linkpath, resolved_link, pid_first);
					return std::strtod(pid_first, nullptr);
				}
			}
		}
		
		closedir(fddir), fddir = nullptr;
	}
	
	EXTLOG_DEBUG_FMT(logger, "find_pid_by_inode: failed to find pid for inode = {}", inode);
	return 0;
}

auto find_socket_inode_procfs(socket_handle_type sock, ext::log::logger * logger) -> std::tuple<uid_t, ino64_t>
{
	return find_socket_pid_helper(find_socket_inode_procfs, "find_socket_inode_procfs", sock, logger);
}

auto find_socket_counter_inode_procfs(int sock, ext::log::logger * logger) -> std::tuple<uid_t, ino64_t>
{
	return find_socket_counter_pid_helper(find_socket_inode_procfs, "find_socket_counter_inode_procfs", sock, logger);
}

#endif // BOOST_OS_UNIX
