#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <type_traits>
#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <chrono>

#include <unordered_set>

#include <ext/itoa.hpp>
#include <ext/future.hpp>
#include <ext/enum_bitset.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>
#include <ext/strings/cow_string.hpp>
#include <ext/strings/compact_string.hpp>
#include <ext/join_into.hpp>
#include <ext/type_traits.hpp>

#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/algorithm/string.hpp>


#include <ext/Errors.hpp>
#include <ext/filesystem_utils.hpp>
#include <ext/netlib/socket_stream.hpp>
#include <ext/netlib/http_parser.hpp>
#include <ext/netlib/http_stream.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/context/detail/apply.hpp>
#include <boost/fiber/all.hpp>
#include "future-fiber.hpp"

#include <boost/timer/timer.hpp>
#include <boost/sort/sort.hpp>

#include <boost/core/demangle.hpp>
#include <boost/mp11.hpp>

#include <ext/netlib/socket_include.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


int main()
{
	using namespace std;

	ext::netlib::socket_streambuf sb;
	sb.timeout(chrono::system_clock::duration::max());

	try
	{
		std::string str;
		std::getline(cin, str);

		sb.connect("localhost", 8080);

		str.resize(1024);
		auto shm = sb.showmanyc();
		auto read = sb.read_some(str.data(), str.size());

		auto sock = sb.handle();
		unsigned user_timeout = 10 * 1'000;
		int res = ::setsockopt(sock, IPPROTO_TCP, TCP_USER_TIMEOUT, &user_timeout, sizeof(user_timeout));
		fmt::print(cout, "setsockopt with TCP_USER_TIMEOUT ended with {} and errno is {}\n", res, ext::FormatErrno(errno));

		cout << flush;

		int ch = sb.sgetc();
		cout << "readed" << endl;
		return 0;
	}
	catch(std::system_error & ex)
	{
		cerr << ext::FormatError(ex) << endl;
	}


}
