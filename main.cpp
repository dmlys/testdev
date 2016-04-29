#include <iostream>
#include <sstream>
#include <fstream>
#include <xstring>
#include <string>
//#include <vector>
//#include <map>
//
//#include <csignal>
//

#include "intrusive_cow_ptr.hpp"
#include "cow_string.hpp"
#include <ext/strings/basic_string_facade.hpp>
#include <ext/strings/basic_string_facade_integration.hpp>

#include <ext/strings/compact_string.hpp>

typedef ext::basic_string_facade<
	ext::cow_string, std::char_traits<char>
> cow_string;


struct Base {};
struct Derived : Base {};

int main()
{
	using namespace std;
	
	typedef cow_string string;
	//typedef std::string string;
	//typedef ext::compact_string string;

	std::string repl = "extxxx";
	string ss = "test str";
	ss.replace(ss.data() + 1, ss.data() + 4, repl.begin(), repl.end());
	//ss.replace(1, 3, 4, 'x');
	//ss.replace(1, 3, string("e"));

	cout << ss << endl;
	cout << ss.use_count() << endl;

	auto p = ss;
	auto z = p;

	cout << z.use_count() << endl;

	z[0] = '0';
	cout << z.use_count() << endl;
	cout << z << endl;
	cout << p.use_count() << endl;
	cout << p << endl;

	z += ss;
	cout << z << endl;

	cout << ss.use_count() << endl;


	return 0;
}
