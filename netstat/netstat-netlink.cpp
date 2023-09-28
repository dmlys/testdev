#include "netstat.hpp"
#include "netstat-helpers.hpp"
#if BOOST_OS_LINUX

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <functional>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/inet_diag.h>
#include <poll.h>

#include <boost/scope_exit.hpp>
#include <fmt/format.h>

#include <ext/errors.hpp>
#include <ext/range/input_range_facade.hpp>
#include <ext/log/logging_macros.hpp>
#include <ext/net/socket_base.hpp>


//Kernel TCP states. /include/net/tcp_states.h
//enum{
//	TCP_ESTABLISHED = 1,
//	TCP_SYN_SENT,
//	TCP_SYN_RECV,
//	TCP_FIN_WAIT1,
//	TCP_FIN_WAIT2,
//	TCP_TIME_WAIT,
//	TCP_CLOSE,
//	TCP_CLOSE_WAIT,
//	TCP_LAST_ACK,
//	TCP_LISTEN,
//	TCP_CLOSING,
//};

//#define TCPF_ALL 0xFFF

static void wait_state(socket_handle_type sock, int millis, int fstate)
{
	int err;
	sockoptlen_t solen;

again:
	pollfd fd;
	fd.events = 0;
	fd.fd = sock;
	if (fstate & 0x1)
		fd.events |= POLLIN;
	if (fstate & 0x2)
		fd.events |= POLLOUT;

	int timeout = millis;
	int res = poll(&fd, 1, timeout);
	if (res == 0) // timeout
		throw std::system_error(std::make_error_code(std::errc::timed_out), "recv_netlink_inet_diag_msg: recvmsg(poll) timeout");

	if (res < 0)
		goto sockerror;
		

	if (fd.revents & POLLERR)
	{
		solen = sizeof(err);
		res = ::getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &solen);
		if (res != 0) goto sockerror;
		if (err != 0) goto error;
	}

	assert(fd.revents & (fd.events | POLLHUP));
	return;

sockerror:
	err = errno;
error:
	if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR)
		goto again;
	
	throw std::system_error(std::error_code(err, std::generic_category()), "recv_netlink_inet_diag_msg: recvmsg(poll) error");
}

static void wait_readable(socket_handle_type sock, int millis) { return wait_state(sock, millis, 0x1); }
static void wait_writable(socket_handle_type sock, int millis) { return wait_state(sock, millis, 0x2); }

template <class ... Type>
static int send_netlink_message(socket_handle_type netlink_sock, Type * ... payload)
{
	constexpr unsigned n = sizeof ...(payload);
	struct iovec iov[n] = {   { payload, sizeof(*payload) }...   };
	
	return send_netlink_message(netlink_sock, n, iov);
}

static int send_netlink_message(socket_handle_type netlink_sock, unsigned niov, struct iovec * iov)
{
	struct sockaddr_nl netlink_addr;
	struct msghdr netlink_msg;
	
	std::memset(&netlink_addr, 0, sizeof netlink_addr);
	std::memset(&netlink_msg, 0, sizeof netlink_msg);
	
	netlink_addr = { .nl_family = AF_NETLINK, .nl_pid = 0 };
	netlink_msg = {
	    .msg_name = &netlink_addr, .msg_namelen = sizeof(netlink_addr),
	    .msg_iov = iov, .msg_iovlen = niov
	};
	
	return ::sendmsg(netlink_sock, &netlink_msg, 0);
}

static void send_netlink_inet_diag_req(socket_handle_type netlink_sock, int sock_proto, const sockaddr * src, const sockaddr * dst, ext::log::logger * logger)
{
	// See man 7 netlink and man sock_diag. (also man sendmsg).
	// We need to create 2 messages: header - nlmsghdr and payload - inet_diag_req_v2,
	// and send them one after another as whole UDP diagram
	
	// our messages
	struct nlmsghdr netlink_msghdr;         // header
	struct inet_diag_req_v2 inet_conn_req;  // payload

	//std::memset(&netlink_addr, 0, sizeof netlink_addr);
	//std::memset(&netlink_msg, 0, sizeof netlink_msg);
	std::memset(&netlink_msghdr, 0, sizeof netlink_msghdr);
	std::memset(&inet_conn_req, 0, sizeof inet_conn_req);
	
	// fill header
	netlink_msghdr =
	{
	    // NOTE: NLMSG_LENGTH adds nlmsghdr size, see macro declaration
	    .nlmsg_len = NLMSG_LENGTH(sizeof inet_conn_req),   /* Length of message including header */
	    // Sets payload type SOCK_DIAG_BY_FAMILY/inet_diag_req_v2
	    .nlmsg_type = SOCK_DIAG_BY_FAMILY,                  /* Message content */
	    // In order to request a socket bound to a specific IP/port, remove
	    // NLM_F_DUMP and specify the required information in conn_req.id
	    .nlmsg_flags = NLM_F_REQUEST /*| NLM_F_DUMP*/,      /* Additional flags */
	    .nlmsg_seq = 0,	                                    /* Sequence number */
	    .nlmsg_pid = 0,	                                    /* Sending process port ID */
	};
	
	// fill payload
	inet_conn_req = {
	    .sdiag_family = static_cast<std::uint8_t>(src->sa_family),
	    .sdiag_protocol = static_cast<std::uint8_t>(sock_proto),
	    //.idiag_ext = (1 << (INET_DIAG_INFO - 1)),   // This is a set of flags defining what kind of extended information to report
	    //.pad = 0,
	    .idiag_states = 0,                          // This is a bit mask that defines a filter of socket states.
	                                                // Only those sockets whose states are in this mask will be reported.
	                                                // Ignored when querying for an individual socket.
	};
	
	// and fill identification
	auto & id = inet_conn_req.id;
	
	// from man sock_diag:
	//   This  is  an  array of opaque identifiers that could be used along with other fields of this structure to specify an individual socket.
	//   It is ignored when querying for a list of sockets, as well as when all its elements are set to -1.
	id.idiag_cookie[0]= -1u;
	id.idiag_cookie[1]= -1u;
	//   The interface number the socket is bound to.
	//   NOTE: Not sure how to specify any interface(search without interface specification).
	//         But looks like 0 is ignored
	id.idiag_if = 0;
	
	// srd and dest
	assert(src->sa_family == dst->sa_family);
	if (src->sa_family == AF_INET)
	{
		auto dst_in = reinterpret_cast<const sockaddr_in *>(dst);
		auto src_in = reinterpret_cast<const sockaddr_in *>(src);

		id.idiag_dport = dst_in->sin_port;
		id.idiag_sport = src_in->sin_port;

		static_assert(sizeof id.idiag_dst == 16);
		std::memset(id.idiag_dst, 0, sizeof id.idiag_dst);
		std::memset(id.idiag_src, 0, sizeof id.idiag_src);
		id.idiag_dst[0] = dst_in->sin_addr.s_addr;
		id.idiag_src[0] = src_in->sin_addr.s_addr;

	}
	else if (src->sa_family == AF_INET6)
	{
		auto dst_in = reinterpret_cast<const sockaddr_in6 *>(dst);
		auto src_in = reinterpret_cast<const sockaddr_in6 *>(src);

		id.idiag_dport = dst_in->sin6_port;
		id.idiag_sport = src_in->sin6_port;

		static_assert(sizeof dst_in->sin6_addr.s6_addr == 16);
		std::memcpy(id.idiag_dst, dst_in->sin6_addr.s6_addr, sizeof dst_in->sin6_addr.s6_addr);
		std::memcpy(id.idiag_src, src_in->sin6_addr.s6_addr, sizeof src_in->sin6_addr.s6_addr);
	}
	
	// send at last
	EXTLOG_TRACE_STR(logger, "send_netlink_inet_diag_req: sending request");
	auto retval = send_netlink_message(netlink_sock, &netlink_msghdr, &inet_conn_req);
	EXTLOG_TRACE_FMT(logger, "send_netlink_inet_diag_req: request sent, retval = {}, errno = {}", retval, ext::format_errno(errno));
	
	if (retval < 0) ext::throw_last_errno("send_netlink_inet_diag_req: sendmsg failed");
}

static void recv_netlink_inet_diag_msg(socket_handle_type netlink_sock, std::function<bool(inet_diag_msg *)> matacher, ext::log::logger * logger)
{
	// This method is called to receive answer for request done by send_netlink_inet_diag_req.
	// Requests specifies particular socket, so there should be only one answer.
	
	std::vector<uint8_t> recv_buffer;
	constexpr unsigned socket_buffer_size = 4096;
	recv_buffer.resize(socket_buffer_size);

	// recvmsg stuff
	struct iovec iov = { recv_buffer.data(), recv_buffer.size() };
	struct sockaddr_nl netlink_addr;
	struct msghdr netlink_msg;

	netlink_msg = {
	    .msg_name = &netlink_addr,
	    .msg_namelen = sizeof netlink_addr,
	    .msg_iov = &iov, .msg_iovlen = 1,
	};
	
	// The messages can (probably will for dump cases) come as multiple netlink messages.
	// Through we expect only one message.
	// NOTE: from man 7 netlink:
	//   Netlink is not a reliable protocol. It tries its best to deliver a message to its destination(s), but may drop messages when an out-of-memory condition or other error occurs.
	//   For reliable transfer the sender can request an acknowledgement from the receiver by setting the NLM_F_ACK flag.
	//   An acknowledgement is an NLMSG_ERROR packet with the error field set to 0.  The application must generate acknowledgements for received messages itself.
	//   The kernel tries to send an NLMSG_ERROR message for every failed packet.  A user process should follow this convention too.
	//
	//   However, reliable transmissions from kernel to user are impossible in any case.
	//   The kernel can't send a netlink message if the socket buffer is full:
	//   the message will be dropped and the kernel and the user-space process will no longer have the same view of kernel state.
	//   It is up to the application to detect when this happens (via the ENOBUFS error returned by recvmsg(2)) and resynchronize.

	bool multipart;
	unsigned nmsg = 0;
	do
	{
		multipart = false;
		
		//wait_readable(netlink_sock, 500);
		EXTLOG_TRACE_STR(logger, "recv_netlink_inet_diag_msg: receiving reply");
		// with MSG_TRUNC recvbytes would return full size of datagram even with truncating 
		auto recvbytes = recvmsg(netlink_sock, &netlink_msg, MSG_TRUNC);
		EXTLOG_TRACE_FMT(logger, "recv_netlink_inet_diag_msg: received reply, recvbytes = {}, errno = {}", recvbytes, ext::format_errno(errno));
		if (recvbytes < 0) ext::throw_last_errno("recv_netlink_inet_diag_msg: recvmsg failed");
		
		bool truncated = netlink_msg.msg_flags & MSG_TRUNC;
		auto msglen = truncated ? recv_buffer.size() : recvbytes;
		auto netlink_msghdr = reinterpret_cast<nlmsghdr *>(recv_buffer.data());
		assert(not truncated);
		
		// default multipart loop
		while(NLMSG_OK(netlink_msghdr, msglen))
		{
			multipart = netlink_msghdr->nlmsg_flags & NLM_F_MULTI;
			EXTLOG_TRACE_FMT(logger, "recv_netlink_inet_diag_msg: processing {} nlmsghdr, type = {}, flags = {:#x}, {}",
			                 nmsg++, netlink_msghdr->nlmsg_type, netlink_msghdr->nlmsg_flags, multipart ? "multipart" : "not multipart");
			
			if(netlink_msghdr->nlmsg_type == NLMSG_DONE)
			{
				assert(not multipart);
				EXTLOG_TRACE_FMT(logger, "recv_netlink_inet_diag_msg: got NLMSG_DONE, breaking");
				break;
			}

			if(netlink_msghdr->nlmsg_type == NLMSG_ERROR)
			{
				auto errmsg = static_cast<nlmsgerr *>(NLMSG_DATA(netlink_msghdr));
				EXTLOG_DEBUG_FMT(logger, "recv_netlink_inet_diag_msg: got NLMSG_ERROR, nlmsgerr->error = {}", errmsg->error);
				
				int err = errmsg->error;
				if (err == 0) // it's an ack, through we didn't request acks, skip it
					continue;
				
				// it looks that error will contain errno, but negative,
				// -2(-ENOENT) is returned when no record found(more study needed)
				if (err == -ENOENT) // not a error, just not found
				{
					EXTLOG_DEBUG_STR(logger, "recv_netlink_inet_diag_msg: ENOENT result");
					return;
				}
				
				std::error_code errc(-err, std::system_category());
				throw std::system_error(errc, "recv_netlink_inet_diag_msg: received error message");
			}
			else if (netlink_msghdr->nlmsg_type == SOCK_DIAG_BY_FAMILY)
			{
				auto * incoming_msg = static_cast<inet_diag_msg *>(NLMSG_DATA(netlink_msghdr));
				//auto rtalen = netlink_msghdr->nlmsg_len - NLMSG_LENGTH(sizeof(*incoming_msg));
				
				if (matacher(incoming_msg))
				{
					EXTLOG_TRACE_STR(logger, "recv_netlink_inet_diag_msg: matched, returning");
					return;
				}
				
				netlink_msghdr = NLMSG_NEXT(netlink_msghdr, msglen);
			}
			else
			{
				EXTLOG_DEBUG_FMT(logger, "recv_netlink_inet_diag_msg: got unexpected message, type = {}, expected = {}, skipping", netlink_msghdr->nlmsg_type, SOCK_DIAG_BY_FAMILY);
			}
		}
		
	} while (multipart);
	
	EXTLOG_TRACE_STR(logger, "recv_netlink_inet_diag_msg: no more messages");
}

template <class Type>
class netlink_range :
        public ext::input_range_facade<netlink_range<Type>, Type *, Type *>
{
	using self_type = netlink_range;
	using base_type = ext::input_range_facade<self_type, Type *, Type *>;
	
private:
	socket_handle_type netlink_sock;
	
	unsigned state : sizeof(unsigned) * CHAR_BIT - 3;
	unsigned exhausted : 1;
	unsigned multipart : 1;
	unsigned truncated : 1;
	
	unsigned nmsg;
	unsigned msglen;
	
	nlmsghdr * netlink_msghdr;
	Type * diag_msg;
	
	std::vector<uint8_t> recv_buffer;
	ext::log::logger * logger = nullptr;
	std::string_view logging_prefix;
	
public:
	bool empty() const { return exhausted; }
	auto front() const { return diag_msg; }
	void pop_front();
	
public:
	netlink_range(socket_handle_type netlink_sock, ext::log::logger * logger = nullptr, std::string_view logging_prefix = "");
};

template <class Type>
netlink_range<Type>::netlink_range(socket_handle_type netlink_sock, ext::log::logger * logger, std::string_view logging_prefix)
    : netlink_sock(netlink_sock), logger(logger), logging_prefix(logging_prefix)
{
	state = 0;
	exhausted = 0;
	multipart = 0;
	truncated = 0;
	nmsg = 0;
	msglen = 0;
	
	constexpr unsigned socket_buffer_size = 4096;
	recv_buffer.resize(socket_buffer_size);
	
	pop_front();
}

template <class Type>
void netlink_range<Type>::pop_front()
{
	constexpr unsigned STATE_START = 0;
	constexpr unsigned STATE_NEXT_MESSAGE = 1;
	constexpr unsigned STATE_EXHAUSTED = 2;
	
	switch (state) 
	{
		default: EXT_UNREACHABLE();
		case STATE_START:;
		do
		{
			// The messages can (probably will for dump cases) come as multiple netlink messages.
			// NOTE: from man 7 netlink:
			//   Netlink is not a reliable protocol. It tries its best to deliver a message to its destination(s), but may drop messages when an out-of-memory condition or other error occurs.
			//   For reliable transfer the sender can request an acknowledgement from the receiver by setting the NLM_F_ACK flag.
			//   An acknowledgement is an NLMSG_ERROR packet with the error field set to 0.  The application must generate acknowledgements for received messages itself.
			//   The kernel tries to send an NLMSG_ERROR message for every failed packet.  A user process should follow this convention too.
			//
			//   However, reliable transmissions from kernel to user are impossible in any case.
			//   The kernel can't send a netlink message if the socket buffer is full:
			//   the message will be dropped and the kernel and the user-space process will no longer have the same view of kernel state.
			//   It is up to the application to detect when this happens (via the ENOBUFS error returned by recvmsg(2)) and resynchronize.
			
			// recvmsg stuff
			struct iovec iov;
			struct sockaddr_nl netlink_addr;
			struct msghdr netlink_msg;
			
			iov = { recv_buffer.data(), recv_buffer.size() };
		
			netlink_msg = {
				.msg_name = &netlink_addr,
				.msg_namelen = sizeof netlink_addr,
				.msg_iov = &iov, .msg_iovlen = 1,
			};
			
			multipart = false;
			
			//wait_readable(netlink_sock, 500);
			EXTLOG_TRACE_FMT(logger, "{}receiving reply", logging_prefix);
			// with MSG_TRUNC recvbytes would return full size of datagram even with truncating 
			unsigned recvbytes;
			recvbytes = recvmsg(netlink_sock, &netlink_msg, MSG_TRUNC);
			EXTLOG_TRACE_FMT(logger, "{}received reply, recvbytes = {}, errno = {}", logging_prefix, recvbytes, ext::format_errno(errno));
			if (recvbytes < 0) ext::throw_last_errno("{}recvmsg failed", logging_prefix);
			
			truncated = netlink_msg.msg_flags & MSG_TRUNC;
			msglen = truncated ? recv_buffer.size() : recvbytes;
			netlink_msghdr = reinterpret_cast<nlmsghdr *>(recv_buffer.data());
			assert(not truncated);
			
			// default multipart loop
			while(NLMSG_OK(netlink_msghdr, msglen))
			{
				multipart = netlink_msghdr->nlmsg_flags & NLM_F_MULTI;
				EXTLOG_TRACE_FMT(logger, "{}processing {} nlmsghdr, type = {}, flags = {:#x}, {}", logging_prefix,
				                 nmsg++, netlink_msghdr->nlmsg_type, netlink_msghdr->nlmsg_flags, multipart ? "multipart" : "not multipart");
				
				if(netlink_msghdr->nlmsg_type == NLMSG_DONE)
				{
					assert(not multipart);
					EXTLOG_TRACE_FMT(logger, "{}got NLMSG_DONE, breaking", logging_prefix);
					exhausted = true; state = STATE_EXHAUSTED;
					return;
				}
	
				if(netlink_msghdr->nlmsg_type == NLMSG_ERROR)
				{
					auto errmsg = static_cast<nlmsgerr *>(NLMSG_DATA(netlink_msghdr));
					EXTLOG_DEBUG_FMT(logger, "{}got NLMSG_ERROR, nlmsgerr->error = {}", logging_prefix, errmsg->error);
					
					int err = errmsg->error;
					if (err == 0) // it's an ack, through we didn't request acks, skip it
						continue;
					
					// it looks that error will contain errno, but negative,
					// -2(-ENOENT) is returned when no record found(more study needed)
					if (err == -ENOENT) // not a error, just not found
					{
						EXTLOG_DEBUG_FMT(logger, "{}ENOENT result", logging_prefix);
						exhausted = true; state = STATE_EXHAUSTED;
						return;
					}
					
					std::error_code errc(-err, std::system_category());
					throw std::system_error(errc, fmt::format("{}received error message", logging_prefix));
				}
				else if (netlink_msghdr->nlmsg_type == SOCK_DIAG_BY_FAMILY)
				{
					diag_msg = static_cast<Type *>(NLMSG_DATA(netlink_msghdr));
					//auto rtalen = netlink_msghdr->nlmsg_len - NLMSG_LENGTH(sizeof(*incoming_msg));
					
					state = STATE_NEXT_MESSAGE;
					return;
					
					case STATE_NEXT_MESSAGE:;
					netlink_msghdr = NLMSG_NEXT(netlink_msghdr, msglen);
				}
				else
				{
					EXTLOG_DEBUG_FMT(logger, "{}got unexpected message, type = {}, expected = {}, skipping", logging_prefix, netlink_msghdr->nlmsg_type, SOCK_DIAG_BY_FAMILY);
				}
			}
			
		} while (multipart);
		
		EXTLOG_TRACE_FMT(logger, "{}no more messages", logging_prefix);
		
		exhausted = true; state = STATE_EXHAUSTED;
		case STATE_EXHAUSTED:;
	}
}

auto find_socket_inode_netlink(int proto, const sockaddr * sock_addr, const sockaddr * peer_addr, ext::log::logger * logger) -> std::tuple<uid_t, ino64_t>
{
	assert(sock_addr);
	assert(peer_addr);
	
	if (proto != IPPROTO_TCP and proto != IPPROTO_UDP)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: unsupported socket protocol = {}", proto_str(proto)));
	
	if (sock_addr->sa_family != peer_addr->sa_family)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: sock_addr->sa_family <> peer_addr->sa_family({} <> {})", sock_addr->sa_family, peer_addr->sa_family));
	
	if (sock_addr->sa_family != AF_INET and sock_addr->sa_family != AF_INET6)
		throw std::runtime_error(fmt::format("find_socket_inode_netlink: bad sock_addr->sa_family = {}, only AF_INET or AF_INET6 supported", sock_addr->sa_family));
	
	
	EXTLOG_DEBUG_FMT(logger, "find_socket_inode_netlink: searching inode for proto = {}, {} <- {}",
	                 proto_str(proto), ext::net::sockaddr_endpoint_noexcept(sock_addr), ext::net::sockaddr_endpoint_noexcept(peer_addr));
	
	auto netlink_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_INET_DIAG);
	if (netlink_sock == ext::net::invalid_socket)
		ext::throw_last_errno("find_socket_inode: failed to create netlink/inet_diag socket");

	BOOST_SCOPE_EXIT_ALL(&netlink_sock)
	{
		if (netlink_sock != ext::net::invalid_socket)
			ext::net::close(netlink_sock);
	};
	
	
	uid_t uid = 0;
	ino64_t inode = 0;
	auto process = [sock_addr, peer_addr, &uid, &inode](inet_diag_msg * msg)
	{
		bool matched;
		if (sock_addr->sa_family == AF_INET)
		{
			auto * sock_addr_in = reinterpret_cast<const sockaddr_in *>(sock_addr),
			     * peer_addr_in = reinterpret_cast<const sockaddr_in *>(peer_addr);
			
			constexpr unsigned ipv4_size = sizeof(in_addr::s_addr);
			static_assert (ipv4_size == 4);
			
			matched = std::memcmp(&sock_addr_in->sin_addr.s_addr, msg->id.idiag_src, ipv4_size) == 0 and sock_addr_in->sin_port == msg->id.idiag_sport
			      and std::memcmp(&peer_addr_in->sin_addr.s_addr, msg->id.idiag_dst, ipv4_size) == 0 and peer_addr_in->sin_port == msg->id.idiag_dport;
		}
		else if (sock_addr->sa_family == AF_INET6)
		{
			auto * sock_addr_in = reinterpret_cast<const sockaddr_in6 *>(sock_addr),
			     * peer_addr_in = reinterpret_cast<const sockaddr_in6 *>(peer_addr);
			
			constexpr unsigned ipv6_size = sizeof(in6_addr::s6_addr);
			static_assert (ipv6_size == 16);
			
			matched = std::memcmp(sock_addr_in->sin6_addr.s6_addr, msg->id.idiag_src, ipv6_size) == 0 and sock_addr_in->sin6_port == msg->id.idiag_sport
			      and std::memcmp(peer_addr_in->sin6_addr.s6_addr, msg->id.idiag_dst, ipv6_size) == 0 and peer_addr_in->sin6_port == msg->id.idiag_dport;
		}
		else
		{
			matched = false;
		}
		
		if (matched)
		{
			uid = msg->idiag_uid;
			inode = msg->idiag_inode;
		}
		
		return matched;
	};
	
	send_netlink_inet_diag_req(netlink_sock, proto, sock_addr, peer_addr, logger);
	//recv_netlink_inet_diag_msg(netlink_sock, process, logger);
	for (auto * msg : netlink_range<inet_diag_msg>(netlink_sock, logger, "recv_netlink_inet_diag_msg: "))
		if (process(msg)) break;
	
	EXTLOG_DEBUG_FMT(logger, "find_socket_inode_netlink: searching inode result: uid = {}, inode = {}", uid, inode);
	
	return std::make_tuple(uid, inode);
}

auto find_socket_inode_netlink(socket_handle_type sock, ext::log::logger * logger) -> std::tuple<uid_t, ino64_t>
{
	return find_socket_pid_helper(find_socket_inode_netlink, "find_socket_inode_netlink", sock, logger);
}

auto find_socket_counter_inode_netlink(int sock, ext::log::logger * logger) -> std::tuple<uid_t, ino64_t>
{
	return find_socket_counter_pid_helper(find_socket_inode_netlink, "find_socket_counter_inode_netlink", sock, logger);
}

#endif // BOOST_OS_LINUX
