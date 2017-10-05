#include <ext/future.hpp>
#include <boost/fiber/all.hpp>
#include "future-fiber.hpp"

namespace ext
{
	enum class thread_type : bool
	{
		thread = true,
		fiber = false,
	};

	static thread_local thread_type gth_thread_type = thread_type::thread;

	void init_future_fiber()
	{
		gth_thread_type = thread_type::fiber;
	}

	void release_future_fiber()
	{
		gth_thread_type = thread_type::thread;
	}


	class fiber_waiter : public ext::continuation_waiter
	{
	private:
		boost::fibers::mutex m_fiber_mutex;
		boost::fibers::condition_variable m_fiber_var;

		std::mutex m_thread_mutex;
		std::condition_variable m_thread_var;

		std::atomic_bool m_ready = ATOMIC_VAR_INIT(false);

	public:
		/// waiting functions: waits until object become ready by execute call
		virtual void wait_ready() noexcept override;
		virtual bool wait_ready(std::chrono::steady_clock::time_point timeout_point) noexcept override;
		virtual bool wait_ready(std::chrono::steady_clock::duration   timeout_duration) noexcept override;

	public:
		/// fires condition_variable, waking any waiting thread
		virtual void continuate(shared_state_basic * caller) noexcept override;
		/// reset waiter, after that it can be used again
		virtual void reset() noexcept override;
	};



	void fiber_waiter::continuate(shared_state_basic * caller) noexcept
	{
		m_ready.store(true, std::memory_order_relaxed);
		m_thread_var.notify_all();
		m_fiber_var.notify_all();
	}

	void fiber_waiter::wait_ready() noexcept
	{
		if (gth_thread_type == thread_type::thread)
		{
			std::unique_lock<std::mutex> lk(m_thread_mutex);
			return m_thread_var.wait(lk, [this] { return m_ready.load(std::memory_order_relaxed); });
		}
		else
		{
			std::unique_lock<boost::fibers::mutex> lk(m_fiber_mutex);
			return m_fiber_var.wait(lk, [this] { return m_ready.load(std::memory_order_relaxed); });
		}
	}

	bool fiber_waiter::wait_ready(std::chrono::steady_clock::time_point timeout_point) noexcept
	{
		if (gth_thread_type == thread_type::thread)
		{
			std::unique_lock<std::mutex> lk(m_thread_mutex);
			return m_thread_var.wait_until(lk, timeout_point, [this] { return m_ready.load(std::memory_order_relaxed); });
		}
		else
		{
			std::unique_lock<boost::fibers::mutex> lk(m_fiber_mutex);
			return m_fiber_var.wait_until(lk, timeout_point, [this] { return m_ready.load(std::memory_order_relaxed); });
		}
	}

	bool fiber_waiter::wait_ready(std::chrono::steady_clock::duration timeout_duration) noexcept
	{
		if (gth_thread_type == thread_type::thread)
		{
			std::unique_lock<std::mutex> lk(m_thread_mutex);
			return m_thread_var.wait_for(lk, timeout_duration, [this] { return m_ready.load(std::memory_order_relaxed); });
		}
		else
		{
			std::unique_lock<boost::fibers::mutex> lk(m_fiber_mutex);
			return m_fiber_var.wait_for(lk, timeout_duration, [this] { return m_ready.load(std::memory_order_relaxed); });
		}
	}

	void fiber_waiter::reset() noexcept
	{
		m_ready.store(false, std::memory_order_relaxed);
		m_fstnext.store(fsnext_init, std::memory_order_relaxed);
		m_promise_state.store(static_cast<unsigned>(ext::future_state::unsatisfied), std::memory_order_relaxed);
	}




	bool fiber_waiter_pool::take(waiter_ptr & ptr)
	{
		ptr = ext::make_intrusive<fiber_waiter>();
		m_usecount.fetch_add(1, std::memory_order_relaxed);
		return true;
	}

	bool fiber_waiter_pool::putback(waiter_ptr & ptr)
	{
		ptr = nullptr;
		m_usecount.fetch_sub(1, std::memory_order_relaxed);
		return true;
	}

	bool fiber_waiter_pool::used() const noexcept
	{
		return m_usecount.load(std::memory_order_relaxed);
	}

}
