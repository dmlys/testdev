#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <future>

#define BOOST_THREAD_VERSION 4
#include <boost/thread/future.hpp>

#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <chrono>

#include <ext/future.hpp>
#include <ext/enum_bitset.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>

#include <fmt/format.h>

#include <ext/strings/cow_string.hpp>

int main()
{
	using namespace std;

	cout << "Boost version: " << BOOST_VERSION << endl;
	ext::init_future_library(16);

	//_CrtMemState state;
	//_CrtMemCheckpoint(&state);
	//
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	//_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

	//_CrtMemDumpAllObjectsSince(&state);

	auto f1 = ext::async(ext::launch::deferred, [] { return 12; }).share();
	ext::future<int> f2;

	{
		ext::thread_pool pool {1};
		f2 = pool.submit(f1, [](auto f) { return f.get() + 12; });
	}

	f2.wait();
	assert(f2.is_abandoned());
	
	f1.wait();
	cout << f1.get() << endl;
	
	return 0;
}
