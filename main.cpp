#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <vector>
#include <random>
#include <functional>
#include <algorithm>
#include <chrono>

#include <ext/enum_bitset.hpp>
#include <ext/thread_pool.hpp>
#include <ext/threaded_scheduler.hpp>

#include <cppformat/format.h>

#ifdef _MSC_VER
#ifdef NDEBUG
#pragma comment(lib, "cppformat-mt.lib")
#else
#pragma comment(lib, "cppformat-mt-gd.lib")
#endif
#endif


int main()
{
	using namespace std;

	ext::init_future_library();

	const std::size_t N = 1000 * 10'000;
	const std::size_t batch_size = 10'000;
	std::vector<unsigned> arr;

	arr.resize(N);

	std::mt19937 eng;
	std::uniform_int_distribution<unsigned> dist;

	//print(cout, "Generating {} data\n", N);

	auto rng = [dist, eng]() mutable { return dist(eng); };
	std::generate_n(arr.data(), N, rng);

	//print(cout, "Generated {} data\n", N);


	auto n = N / batch_size;

	ext::thread_pool thp {4};
	std::vector<ext::shared_future<const unsigned *>> farr(n);

	auto start = std::chrono::steady_clock::now();

	for (std::size_t i = 0; i < n; ++i)
	{
		const unsigned * first = arr.data() + i * batch_size;
		const unsigned * last = first + batch_size;

		auto func = [first, last] { 
			return std::max_element(first, last);
		};
		farr[i] = thp.submit(func);
	}

	auto all = ext::when_all(farr.begin(), farr.end());
	all.wait();

	auto pred = [](auto & f1, auto & f2) { return *f1.get() < *f2.get(); };
	auto res = std::max_element(farr.begin(), farr.end(), pred);

	auto total = std::chrono::steady_clock::now() - start;

	//print(cout, "max element is: {}\n", *res->get());
	cout << "max element is: " << *res->get() << endl;
	cout << "max element is: " << *std::max_element(arr.begin(), arr.end()) << endl;
	cout << "took " << std::chrono::duration_cast<std::chrono::milliseconds>(total).count() << "ms" << endl;	

	return 0;
}
