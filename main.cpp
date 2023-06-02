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
#include <ext/net/http/http_parser.hpp>
#include <ext/net/http/http_server.hpp>
#include <ext/net/http/zlib_filter.hpp>
#include <ext/net/http/cors_filter.hpp>
//#include <ext/net/http/filesystem_handler.hpp>

//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>


#include <ext/base64.hpp>
#include <ext/net/mime/url_encoding.hpp>


#include <ext/stream_filtering.hpp>
#include <ext/stream_filtering/zlib.hpp>
#include <ext/stream_filtering/basexx.hpp>


//std::atomic_int counter = 0;
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
	ext::net::init_socket_library();
	
	ext::log::ostream_logger logger(clog, ext::log::Info);
	ext::net::http::http_server server;
	g_server = &server;

	server.set_logger(&logger);
	server.set_request_logging_level(ext::log::Trace);

	server.add_handler("/test", [] { return "test"; });
	server.add_handler("/stream", [] (std::unique_ptr<std::streambuf> & sb) { return std::move(sb); });
	
	//server.add_handler(std::make_unique<ext::net::http::filesystem_handler>("/file", "/home/dima/projects/dmlys/"));
	
	server.add_listener(ext::net::listener(8080, AF_INET6));
	server.add_listener(ext::net::listener(8081, AF_INET6));

	server.join();
	
	return 0;
}



//#define __cpp_impl_coroutine 1
//#include <concepts>
//#include <coroutine>
//#include <exception>
//#include <iostream>
//
//struct ReturnObject {
//  struct promise_type {
//    ReturnObject get_return_object() { return {}; }
//    std::suspend_never initial_suspend() { return {}; }
//    std::suspend_never final_suspend() noexcept { return {}; }
//    void unhandled_exception() {}
//  };
//};
//
//struct Awaiter {
//  std::coroutine_handle<> *hp_;
//  constexpr bool await_ready() const noexcept { return false; }
//  void await_suspend(std::coroutine_handle<> h) { *hp_ = h; }
//  constexpr void await_resume() const noexcept {}
//};
//
//ReturnObject
//counter(std::coroutine_handle<> *continuation_out)
//{
//  Awaiter a{continuation_out};
//  for (unsigned i = 0;; ++i) {
//    co_await a;
//    std::cout << "counter: " << i << std::endl;
//  }
//}
//
//void
//main1()
//{
//  std::coroutine_handle<> h;
//  counter(&h);
//  for (int i = 0; i < 3; ++i) {
//    std::cout << "In main1 function\n";
//    h();
//  }
//  h.destroy();
//}
//
//int main()
//{
//	main1();
//}
//
//
