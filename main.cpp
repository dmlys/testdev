#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <ext/intrusive_ptr.hpp>
#include "cow_string_body.hpp"
#include <ext/strings/basic_string_facade.hpp>
#include <ext/strings/basic_string_facade_integration.hpp>

#include <ext/strings/compact_string.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

typedef ext::basic_string_facade<
	ext::cow_string_body, std::char_traits<char>
> cow_string;

int main()
{
	using namespace std;	

	cow_string ss = "123";
	auto ss2 = ss;

	auto ss3 = ss2;

	ss3 = ss + ss2;

	cin >> ss3;
	cout << ss3 << endl;

	return 0;
}
