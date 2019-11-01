#include <iostream>
#include <string>
#include <memory>

#include <ext/Errors.hpp>
#include <ext/net/socket_include.hpp>
#include <ext/net/socket_stream.hpp>
#include <sys/un.h>

int unix_domain_socket_example()
{
	struct sockaddr_un addr;
	std::memset(&addr, 0, sizeof(addr));

	addr.sun_family = AF_UNIX;
	std::strncpy(addr.sun_path, "/tmp/test.socket", sizeof(addr.sun_path) - 1);

	int sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
	int res = ::connect(sockfd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
	if (res == -1)
	{
		std::cerr << ext::FormatErrno(errno) << std::endl;
		return EXIT_FAILURE;
	}

	ext::net::socket_stream ofs {ext::net::socket_streambuf(sockfd)};
	ofs << "Hello world\n";

	ofs.close();

	return EXIT_SUCCESS;
}
