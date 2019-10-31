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
#include <ext/net/http/http_server.hpp>

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>

#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

std::atomic_int counter = 0;

auto test(ext::net::http::http_request & req, ext::thread_pool & pool)
{
	return pool.submit([]
	{
		ext::net::http::http_response resp;
		resp.http_code = 404;
		resp.body = "Not Found ";

		ext::itoa_buffer<int> buffer;
		resp.body += ext::itoa(counter.fetch_add(1, std::memory_order_relaxed), buffer);
		resp.body += "\n";

		return resp;
	});
}

ext::net::http::http_server * g_server = nullptr;

void sig(int)
{
	g_server->interrupt();
}

int main()
{
	using namespace std;
	std::signal(SIGPIPE, SIG_IGN);
	std::signal(SIGINT, sig);

	ext::library_logger::stream_logger logger(std::clog, ext::library_logger::Info);

	auto pool = std::make_unique<ext::thread_pool>();
	pool->set_nworkers(4);

	ext::net::http::http_server server;
	g_server = &server;
	server.set_logger(&logger, true);
	server.set_request_body_logging_level(ext::library_logger::Trace);
	server.set_socket_timeout(10s);
	server.add_listener(8080);
	server.add_handler("/test", std::bind(test, std::placeholders::_1, ref(*pool)));
	server.set_thread_pool(std::move(pool));
	server.join_thread();

	//pool->stop();

	cout << counter << endl;

	return 0;
}
