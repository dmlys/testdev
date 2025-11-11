#include <cstdint>
#include <cassert>
#include <csignal>

#include <iostream>

#include <set>
#include <string>
#include <format>
#include <optional>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/ranges.h>

#include <boost/algorithm/string.hpp>

#include <ext/future.hpp>
#include <ext/log/logger.hpp>
#include <ext/net/parse_url.hpp>
#include <ext/net/http/http_server.hpp>
#include <ext/filesystem_utils.hpp>

#include <ext/openssl.hpp>
#include <openssl/bio.h>
#include <openssl/x509.h>


// #include <cstdint>
// #include <cassert>
// #include <csignal>
// #include <type_traits>
// #include <atomic>

// #include <vector>
// #include <chrono>
// #include <string>

// #include <thread>
// #include <latch>
// #include <barrier>


// static void backoff() noexcept
// {
// 	std::this_thread::yield(); // there can be better options than yield
// }

// static void lock_ptr(std::atomic_uintptr_t & ptr) noexcept
// {
// 	std::uintptr_t val = 0;
// 	while (not ptr.compare_exchange_weak(val, 0x1u, std::memory_order_acquire, std::memory_order_relaxed))
// 	{
// 		val = 0;
// 		backoff();
// 	}
	
// 	assert(ptr.load(std::memory_order_relaxed) == 0x1);
// }

// static void unlock_ptr(std::atomic_uintptr_t & ptr) noexcept
// {
// 	// head is locked
// 	assert(ptr.load(std::memory_order_relaxed) == 0x1);
// 	ptr.store(0, std::memory_order_release);
// }


// int main()
// {
// 	using namespace std;
		
// 	std::atomic_uintptr_t atomic = 0;
// 	std::latch l(2);
	
// 	auto proc = [](std::atomic_uintptr_t & atomic, std::latch & l)
// 	{
// 		l.arrive_and_wait();
		
// 		lock_ptr(atomic);
		
// 		using namespace std::chrono_literals;
// 		std::this_thread::sleep_for(1s);
// 		unlock_ptr(atomic);
		
// 	};
	
// 	std::thread t1(proc, std::ref(atomic), std::ref(l));
// 	std::thread t2(proc, std::ref(atomic), std::ref(l));
	
// 	t1.join();
// 	t2.join();
	
// 	return 0;
// }



//#define __cpp_impl_coroutine 1
//#include <concepts>
//#include <coroutine>
//#include <exception>
//#include <iostream>
//
//struct ReturnObject {
//  struct promise_type {
//    ReturnObject get_return_object() { return {}; }
//    std::suspend_never initial_suspend() { return {}; }
//    std::suspend_never final_suspend() noexcept { return {}; }
//    void unhandled_exception() {}
//  };
//};
//
//struct Awaiter {
//  std::coroutine_handle<> *hp_;
//  constexpr bool await_ready() const noexcept { return false; }
//  void await_suspend(std::coroutine_handle<> h) { *hp_ = h; }
//  constexpr void await_resume() const noexcept {}
//};
//
//ReturnObject
//counter(std::coroutine_handle<> *continuation_out)
//{
//  Awaiter a{continuation_out};
//  for (unsigned i = 0;; ++i) {
//    co_await a;
//    std::cout << "counter: " << i << std::endl;
//  }
//}
//
//void
//main1()
//{
//  std::coroutine_handle<> h;
//  counter(&h);
//  for (int i = 0; i < 3; ++i) {
//    std::cout << "In main1 function\n";
//    h();
//  }
//  h.destroy();
//}
//
//int main()
//{
//	main1();
//}
//
//
