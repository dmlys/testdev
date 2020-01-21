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

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <ext/itoa.hpp>
#include <ext/future.hpp>
#include <ext/thread_pool.hpp>

#include <ext/net/socket_stream.hpp>
#include <ext/net/socket_include.hpp>
#include <ext/net/http_parser.hpp>
#include <ext/net/http/http_server.hpp>
#include <ext/net/http/zlib_filter.hpp>
#include <ext/net/http/cors_filter.hpp>

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>

#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

//#include <log4cplus/log4cplus.h>
//#include <ext/library_logger/log4cplus_logger.hpp>

std::atomic_int counter = 0;

ext::net::http::http_server * g_server = nullptr;

void sig(int)
{
	g_server->interrupt();
}

int main()
{
	using namespace std;

	ext::library_logger::stream_logger logger(clog);
	ext::net::http::http_server server;
	g_server = &server;

	g_server->set_logger(&logger);
	g_server->add_handler("/test", [](std::string & body) { return "test"; });
	g_server->add_handler("/zipper", [](std::string & body) { return "zipper"; });

	g_server->add_filter(ext::make_intrusive<ext::net::http::zlib_filter>());
	g_server->add_filter(ext::make_intrusive<ext::net::http::cors_filter>());

	g_server->add_listener(8080);
	g_server->add_listener(8081);

	server.join_thread();

	return 0;
}
