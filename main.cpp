#include <csignal>
#include <type_traits>
#include <random>
#include <functional>
#include <algorithm>

#include <vector>
#include <chrono>
#include <string>
#include <string_view>

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <ext/itoa.hpp>
#include <ext/errors.hpp>
#include <ext/future.hpp>
#include <ext/thread_pool.hpp>
#include <ext/cppzlib.hpp>

#include <boost/range.hpp>
#include <boost/range/as_literal.hpp>
#include <ext/range.hpp>
#include <ext/filesystem_utils.hpp>

#include <ext/net/socket_stream.hpp>
#include <ext/net/socket_include.hpp>
#include <ext/net/http_parser.hpp>
#include <ext/net/http/http_server.hpp>
#include <ext/net/http/zlib_filter.hpp>
#include <ext/net/http/cors_filter.hpp>

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>


std::atomic_int counter = 0;
ext::net::http::http_server * g_server = nullptr;

void sig(int)
{
	g_server->interrupt();
}

int main()
{
	using namespace std;	
	using namespace chrono;
	
	std::signal(SIGINT, sig);
	
	ext::init_future_library();
	ext::net::socket_stream_init();
	
	ext::library_logger::stream_logger logger(clog, ext::library_logger::Trace);
	ext::net::http::http_server server;
	g_server = &server;

	server.set_logger(&logger);
	server.set_request_logging_level(ext::library_logger::Trace);

	server.add_handler("/test", [] { return "test"; });
	server.add_handler("/stream", [] (std::unique_ptr<std::streambuf> & sb) { return std::move(sb); });
	
	server.add_listener(ext::net::listener(8080, AF_INET6));
	server.add_listener(ext::net::listener(8081, AF_INET6));

	server.join();
	
	return 0;
}
