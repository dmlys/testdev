#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <csignal>

#include <ext/Errors.hpp>
#include <ext/iostreams/socket_stream.hpp>


int main()
{
	using namespace std;
	

	ext::socket_stream_init();
	ext::socket_stream ss;
	ss.exceptions(std::ios::failbit | std::ios::badbit);

	try {
		ss.connect("httpbin.org", "http");
		//ss.start_ssl();

		ss << "GET /get HTTP/1.1\r\n"
			<< "Host: httpbin.org\r\n"
			<< "Connection: close\r\n"
			<< "";


		std::string line;
		ss >> line;
		cout << ss.rdbuf() << endl;
	}

	catch (std::ios::failure &)
	{
		cerr << ext::FormatError(ss.last_error()) << endl;
	}

	return 0;
}

