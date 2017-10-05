#include <ext/future.hpp>

namespace ext
{
	/// sets future fiber
	void init_future_fiber();
	void release_future_fiber();


	class fiber_waiter_pool : public ext::continuation_waiters_pool
	{
	protected:
		std::atomic_uint m_usecount = ATOMIC_VAR_INIT(0);

	public:
		bool take(waiter_ptr & ptr) override;
		bool putback(waiter_ptr & ptr) override;
		bool used() const noexcept override;
	};
}
