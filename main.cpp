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

int main()
{
	using namespace std;

	ext::netlib::socket_streambuf sb;

	try
	{
		sb.connect("localhost", 8080);
		if (sb.is_open()) cout << "success\n";
		else              cout << "not so success\n";
	}
	catch (std::system_error & ex)
	{
		cerr << ex.what() << endl;
		cerr << ext::FormatError(ex) << endl;
		cerr << ext::FormatError(ex.code()) << endl;
	}
}
