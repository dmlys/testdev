#include <cassert>
#include <ext/future.hpp>

void ext_future_test()
{
	{
		auto f1 = ext::async(ext::launch::deferred, [] { return 12; });
		auto f2 = f1.then([](auto f)
		{
			auto val = f.get();
			return ext::async(ext::launch::deferred, [val] { return val * 2 + 3; });
		});

		auto fall = ext::when_all(f2.unwrap());
		ext::future<int> fi = std::get<0>(fall.get());
		assert(fi.get() == 12 * 2 + 3);
	}

}
