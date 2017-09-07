#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <chrono>

#include <variant>
#include <optional>

#include <ext/itoa.hpp>
#include <ext/future.hpp>
#include <ext/enum_bitset.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>
#include <ext/strings/cow_string.hpp>

#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#include <ext/Errors.hpp>
#include <ext/FileSystemUtils.hpp>
#include <ext/iostreams/socket_stream.hpp>
#include <ext/netlib/http_response_parser.hpp>
#include <ext/netlib/http_response_stream.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>


int main()
{
	using namespace std;

	auto fi = ext::async(ext::launch::deferred, [] { return 12; });
	auto ff = fi.then([](auto f) { return f.get() + 100; });

	assert(not ff.has_value());

	auto res = ff.get();
	cout << res << endl;
	return res;
}
