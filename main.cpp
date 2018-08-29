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

#include <ext/itoa.hpp>
#include <ext/future.hpp>
#include <ext/enum_bitset.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>
#include <ext/strings/cow_string.hpp>
#include <ext/join_into.hpp>
#include <ext/type_traits.hpp>


#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/algorithm/string.hpp>


#include <ext/Errors.hpp>
#include <ext/FileSystemUtils.hpp>
#include <ext/iostreams/socket_stream.hpp>
#include <ext/netlib/http_parser.hpp>
#include <ext/netlib/http_stream.hpp>

//#include <ext/library_logger/logger.hpp>
//#include <ext/library_logger/logging_macros.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

//#include <boost/context/detail/apply.hpp>
//#include <boost/fiber/all.hpp>
//#include "future-fiber.hpp"

int main()
{
	using namespace std;


    //ext::init_future_library(std::make_unique<ext::fiber_waiter_pool>());
    //ext::init_future_fiber();

	ext::promise<int> pi1, pi2;
	ext::future<void> f1, f2;

	{
		ext::thread_pool pool;
		pool.set_nworkers(std::thread::hardware_concurrency());

		pool.submit([] { cout << "Hello there\n"; });

		f1 = pool.submit(pi1.get_future(), [](auto f) { cout << f.get() << endl; });
		f2 = pool.submit(pi2.get_future(), [](auto f) { cout << f.get() << endl; });
		
		pi1.set_value(12);
		pi2.set_value(24);

		f1.wait();
		f2.wait();

		auto fs1 = pool.stop();
		auto fs2 = pool.stop();

		when_all(move(fs1), move(fs2)).wait();
	}

	f1.wait();
	f2.wait();

	//ext::init_future_library(std::make_unique<ext::fiber_waiter_pool>());
	//ext::init_future_fiber();

	//ext::packaged_task<int()> task1, task2;

	//task1 = [] { return 12; };
	//task2 = [] { return 100; };

	//auto f1 = task1.get_future();
	//auto f2 = task2.get_future();
	//auto f3 = ext::async(ext::launch::async, [] { return -50; });

	////auto f3 = ext::make_ready_future(12);

	//boost::fibers::fiber ft1 {boost::fibers::launch::dispatch, std::move(task1)};
	//boost::fibers::fiber ft2 {boost::fibers::launch::dispatch, std::move(task2)};

	//auto fres = ext::when_all(std::move(f1), std::move(f2), std::move(f3))
	//	.then([](auto ff)
	//{
	//	ext::future<int> fr1, fr2, fr3;
	//	std::tie(fr1, fr2, fr3) = ff.get();
	//	return fr1.get() + fr2.get() + fr3.get();
	//});
	//
	//cout << fres.get() << endl;
	//
	//ft1.join();
	//ft2.join();

	return 0;
}
