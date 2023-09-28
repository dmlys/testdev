#pragma once
#include <string>
#include <ext/log/logger.hpp>
#include <ext/errors.hpp>
#include <fmt/format.h>

#include <ext/net/socket_fwd.hpp>
#include <ext/net/socket_include.hpp>
#include <ext/net/socket_base.hpp>


std::string proto_str(int proto);

template <class Type>
Type find_socket_pid_helper(
        Type (* finder)(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger),
        const char * function_name, socket_handle_type sock, ext::log::logger * logger)
{
	if (sock <= 0)
	{
		auto errmsg = fmt::format("{}: bad socket", function_name);
		throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor), errmsg);
	}
	
	sockaddr_storage sock_addr_storage, peer_addr_storage;
	socklen_t sock_addr_len = sizeof(sock_addr_storage), peer_addr_len = sizeof(peer_addr_storage);

	auto sock_addr = reinterpret_cast<sockaddr *>(&sock_addr_storage),
	     peer_addr = reinterpret_cast<sockaddr *>(&peer_addr_storage);

	ext::net::getsockname(sock, sock_addr, &sock_addr_len);
	ext::net::getpeername(sock, peer_addr, &peer_addr_len);

	//int proto;
	//sockoptlen_t solen = sizeof proto;
	//int res = getsockopt(sock, SOL_SOCKET, SO_PROTOCOL, &proto, &solen);
	//if (res < 0)
	//{
	//	auto last_err = ext::last_system_error();
	//	auto errmsg = fmt::format("{}: getsockopt(SO_PROTOCOL) failed", function_name);
	//	throw std::system_error(last_err, errmsg);
	//}
	
	return finder(IPPROTO_TCP, sock_addr, peer_addr, logger);
}

template <class Type>
Type find_socket_counter_pid_helper(
        Type (* finder)(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger),
        const char * function_name, socket_handle_type sock, ext::log::logger * logger)
{
	if (sock <= 0)
	{
		auto errmsg = fmt::format("{}: bad socket", function_name);
		throw std::system_error(std::make_error_code(std::errc::bad_file_descriptor), errmsg);
	}
	
	sockaddr_storage sock_addr_storage, peer_addr_storage;
	socklen_t sock_addr_len = sizeof(sock_addr_storage), peer_addr_len = sizeof(peer_addr_storage);

	auto sock_addr = reinterpret_cast<sockaddr *>(&sock_addr_storage),
	     peer_addr = reinterpret_cast<sockaddr *>(&peer_addr_storage);

	ext::net::getsockname(sock, sock_addr, &sock_addr_len);
	ext::net::getpeername(sock, peer_addr, &peer_addr_len);

	//int proto;
	//sockoptlen_t solen = sizeof proto;
	//int res = getsockopt(sock, SOL_SOCKET, SO_PROTOCOL, &proto, &solen);
	//if (res < 0)
	//{
	//	auto last_err = ext::last_system_error();
	//	auto errmsg = fmt::format("{}: getsockopt(SO_PROTOCOL) failed", function_name);
	//	throw std::system_error(last_err, errmsg);
	//}
	
	return finder(IPPROTO_TCP, peer_addr, sock_addr, logger);
}

