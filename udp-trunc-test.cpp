#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <string>

#include <fmt/format.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>


int main()
{
	try
	{
		int u1 = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		int u2 = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		
		assert(u1 > 0);
		assert(u2 > 0);
		
		sockaddr_in addr;
		std::memset(&addr, 0, sizeof addr);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(8181);
		
		int r, b;
		r = bind(u1, reinterpret_cast<sockaddr *>(&addr), sizeof addr);
		assert(r == 0);
		
		r = connect(u2, reinterpret_cast<sockaddr *>(&addr), sizeof addr);
		assert(r == 0);
		
		struct iovec snd_iov, rcv_iov;
		struct msghdr snd_msg, rcv_msg;
		
		std::memset(&snd_msg, 0, sizeof snd_msg);
		std::memset(&rcv_msg, 0, sizeof rcv_msg);
		
		snd_msg = { .msg_iov = &snd_iov, .msg_iovlen = 1 };
		rcv_msg = { .msg_iov = &rcv_iov, .msg_iovlen = 1 };
		
		std::string s1, s2;
		s1 = "Hello world";
		s2.resize(s1.size() - 2);
		
		snd_iov = { .iov_base = s1.data(), .iov_len = s1.size() };
		rcv_iov = { .iov_base = s2.data(), .iov_len = s2.size() };
		
		b = sendmsg(u2, &snd_msg, 0);
		assert(b > 0);
		
		// ioctl with FIONREAD can be used to get full size of net datagram before issuing a recv/recvmsg call
		int datagram_length;
		r = ioctl(u1, FIONREAD, &datagram_length);
		assert(r >= 0); // != -1
		
		// on linux MSG_TRUNC for UDP and other datagram protocols 
		// forces recv and recvmsg to return full datagram size even if truncated.
		// In this example:
		//   recvmsg without MSG_TRUNC would return 9 
		//   recvmsg with    MSG_TRUNC would return 11 
		b = recvmsg(u1, &rcv_msg, MSG_TRUNC); 
		assert(b > 0);
		
		bool truncated = rcv_msg.msg_flags & MSG_TRUNC;
		auto len = truncated ? s2.size() : b;
		s2.resize(len);

		printf("received message, recvbytes = %d, datagram_length = %d, %s, payload = %s\n", b, datagram_length, truncated ? "truncated" : "not truncated", s2.c_str());
		
		return 0;
	}
	catch (std::exception & ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}
}