#include <iostream>
#include <csignal>
#include <thread>

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <ext/errors.hpp>
#include <ext/net/http_parser.hpp>
#include <ext/net/listener.hpp>
#include <ext/net/socket_rest_supervisor.hpp>

#include <ext/library_logger/logger.hpp>


class test_source : public virtual ext::net::socket_rest_supervisor
{
public:
	ext::future<std::string> test_request();

public:
	test_source() = default;
	~test_source() = default;
};


class test_request : public test_source::request<std::string>
{
	void request(ext::net::socket_streambuf & socket)
	{
		static unsigned count = 0;

		std::ostream os(&socket);

		char buffer[1024];
		std::fill_n(buffer, 1024, '1');

		os << "POST /" << count++ << " HTTP/1.1\r\n"
		   << "Connection: keep-alive\r\n"
		   << "Content-Length: 1024\r\n"
		   << "Host: " << host() << "\r\n"
		   << "\r\n";

		os.write(buffer, 1024);
	}

	void response(ext::net::socket_streambuf & streambuf)
	{
		std::string name, body;
		ext::net::http_parser parser(ext::net::http_parser::response);
		parser.parse_status(streambuf, body);
		parser.parse_trailing(streambuf);

		set_value(body);
	}
};

ext::future<std::string> test_source::test_request()
{
	return this->add_request(ext::make_intrusive<::test_request>());
}



int list_thread()
{
	using namespace std;

	ext::net::listener list;
	list.bind(8080);
	list.listen(1);

	ext::net::socket_streambuf recv = list.accept();
	recv.timeout(std::chrono::steady_clock::duration::max());
	recv.throw_errors(true);
	recv.self_tie(false);

	std::string method, uri, body;

	try
	{
		for (;;)
		{
			auto res = recv.sgetc();
			if (res == EOF) break;

			ext::net::parse_http_request(recv, method, uri, body);
			fmt::print("Got request: {} {}\n", method, uri);

			std::ostream os(&recv);
			os << "HTTP/1.1 200 OK\r\n"
			   << "Content-Length: 12\r\n"
			   << "Content-Type: text/plain\r\n"
			   << "\r\n"
			   << "Hello world!";

			os.flush();
		}
	}
	catch (std::runtime_error & ex)
	{
		cerr << ex.what() << std::endl;
		cerr << ext::format_error(recv.last_error()) << std::endl;
	}

	fmt::print(cerr, "listener thread finished\n");
	return 0;
}


int socket_rest_subscriber_main()
{
	using namespace std;
	std::signal(SIGPIPE, SIG_IGN);

	ext::net::socket_stream_init();
	ext::init_future_library(std::thread::hardware_concurrency());

	std::thread thr(list_thread);

	ext::library_logger::stream_logger lg(std::cout);

	test_source source;
	source.set_request_slots(99999);
	source.set_address("localhost", "8080");
	source.set_logger(&lg);

	ext::promise<void> error_promise;
	ext::future<void> ferror = error_promise.get_future();
	source.on_event([&error_promise](auto event)
	{
		if (event == test_source::connection_error)
			error_promise.set_value();
	});

	source.connect();

	std::vector<ext::future<std::string>> futures;
	for (unsigned u = 0; u < 99999; ++u)
		futures.push_back(source.test_request());

	ferror.get();
	fmt::print(cerr, "source error: {}\n", source.last_errormsg());

	//try
	//{
	//	for (auto & f : futures)
	//	{
	//		std::string res = f.get();
	//		fmt::print("res is {}\n", res);
	//	}
	//}
	//catch (std::exception & ex)
	//{
	//	cerr << ex.what() << endl;
	//}

	source.disconnect().get();
	thr.join();
	return 0;
}

