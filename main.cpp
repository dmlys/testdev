#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <ext/threaded_scheduler.hpp>

int main()
{
	using namespace std;

	ext::threaded_scheduler sc;

	auto f1 = sc.add(100ms, [] { cout << "first\n"; }).share();
	auto f2 = sc.add(10s, [] { cout << "second\n"; }).share();

	auto fall = ext::when_all(f1, f2);
	f2.cancel();

	fall.wait();

	return 0;
}
