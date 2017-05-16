#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <chrono>

#include <ext/enum_bitset.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>

#include <ext/iostreams/socket_stream.hpp>
#include <ext/Errors.hpp>

#include <fmt/format.h>

#ifdef _MSC_VER
#ifdef NDEBUG
#pragma comment(lib, "libfmt-mt.lib")
#else
#pragma comment(lib, "libfmt-mt-gd.lib")
#pragma comment(lib, "openssl-crypto-mt-gd.lib")
#pragma comment(lib, "openssl-ssl-mt-gd.lib")
#endif
#endif


int main()
{
	using namespace std;


	ext::init_future_library();
	ext::socket_stream_init();

	ext::socket_stream sock;

	std::string host = "httpbin.org";
	std::string service = "https";
	sock.connect(host, service);
	sock.start_ssl(host);

	if (not sock)
	{
		cerr << ext::FormatError(sock.last_error()) << endl;
		return -1;
	}

	sock << "GET /get HTTP/1.1\r\n"
		<< "Host: httpbin.org\r\n"
		<< "Connection: close\r\n"
		<< "\r\n";

	std::string str;
	while (std::getline(sock, str))
		cout << str << "\n";

	cout << endl;

	return 0;
}
