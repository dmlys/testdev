#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
//#include <vector>
//#include <map>
//
//#include <csignal>
//

#include <ext/intrusive_cow_ptr.hpp>
#include "cow_string_body.hpp"
#include <ext/strings/basic_string_facade.hpp>
#include <ext/strings/basic_string_facade_integration.hpp>

#include <ext/strings/compact_string.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

typedef ext::basic_string_facade<
	ext::cow_string_body, std::char_traits<char>
> cow_string;

struct TV : ext::intrusive_cow_plain_counter<TV>
{
	std::string str;
	
	virtual ~TV() { std::cout << "TV\n"; }
	virtual TV * clone(const TV * tv) const { return new TV(*tv); }

	friend unsigned intrusive_ptr_release(TV * ptr)
	{
		intrusive_ptr_release(static_cast<ext::intrusive_cow_plain_counter<TV> *>(ptr));
		return 0;
	}

	template <class Type>
	friend void intrusive_ptr_clone(const TV * ptr, Type * & dest)
	{
		dest = static_cast<Type *>(ptr->clone(ptr));
	}
};

struct Higher : TV
{
	int clop;

	virtual ~Higher() { std::cout << "Higher\n"; }
	virtual Higher * clone(const TV * tv) const override { return new Higher(static_cast<const Higher &>(*tv)); }
};

int main()
{
	using namespace std;	

	ext::intrusive_cow_ptr<Higher> thp;
	thp.reset(new Higher);

	auto c = thp.use_count();
	cout << sizeof(thp) << endl;
	cout << typeid(c).name() << endl;

	ext::intrusive_cow_ptr<TV> tvp = thp;

	ext::dynamic_pointer_cast<Higher>(tvp);

	thp.detach();
	tvp.detach();
	auto r = thp.release();
	r = thp.reset();

	cow_string ss = "123";

	return 0;
}
