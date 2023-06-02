#pragma once
#include <boost/predef.h>
#include <ext/log/logger.hpp>

#if BOOST_OS_UNIX
#include <cstdint>
#include <tuple>

#include <sys/types.h>
#include <sys/socket.h>

/// Searches socket inode(and also uid) for given sock_addr and peer_addr with help of procfs.
/// If not found -1, -1 is returned
/// Scans and parses content of /proc/net/tcp or /proc/net/tcp6 or /proc/net/upd or /proc/net/udp6)
auto find_socket_inode_procfs(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger = nullptr) -> std::tuple<uid_t, ino64_t>;
/// Searches socket inode(and also uid) for given sock_addr and peer_addr with help of netlink protocol.
/// If not found -1, -1 is returned
auto find_socket_inode_netlink(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger = nullptr) -> std::tuple<uid_t, ino64_t>;

/// Searches socket inode(and also uid) for given sock_addr and peer_addr with help of with help of procfs or netlink(implementation defined).
/// If not found -1, -1 is returned
inline auto find_socket_inode(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger = nullptr) -> std::tuple<uid_t, ino64_t>
{ return find_socket_inode_netlink(proto, sock_addr, peer_addr, logger); }


/// Searches counter socket inode for given socket
/// Basicly calls find_socket_inode with sock_addr and peer_addr inverted from sock arg.
/// In pseudocode: return find_socket_inode(proto, getpeername(sock), getsockname(sock), logger);
auto find_counter_socket_inode_procfs(int sock, ext::log::logger * logger = nullptr) -> std::tuple<uid_t, ino64_t>;
auto find_counter_socket_inode_netlink(int sock, ext::log::logger * logger = nullptr) -> std::tuple<uid_t, ino64_t>;

inline auto find_counter_socket_inode(int sock, ext::log::logger * logger = nullptr) -> std::tuple<uid_t, ino64_t> 
{ return find_counter_socket_inode_netlink(sock, logger); }

/// Searches process id for given inode. Implementation scans procfs, more precisly /proc/$pid/fd/$h -> "socket[$inode]",
/// if found pid is returned(first found), otherwise -1
pid_t find_pid_by_inode(ino64_t inode, ext::log::logger * logger = nullptr);


/// Searches counter socket pid for given socket. Basicly calls find_countet_socket_inode and find_pid_by_inode, if not found -1
inline pid_t find_counter_socket_pid(int sock, ext::log::logger * logger = nullptr)
{
	auto [uid, inode] = find_counter_socket_inode(sock, logger);
	return find_pid_by_inode(inode);
}

#elif BOOST_OS_WINDOWS
#include <ext/net/socket_fwd.hpp>

unsigned int find_socket_pid(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger = nullptr);
unsigned int find_counter_socket_pid(int sock, ext::log::logger * logger = nullptr);

#endif
