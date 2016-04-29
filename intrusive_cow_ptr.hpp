#pragma once
#include <atomic>

namespace ext
{
	template <class Derived>
	class cow_atomic_base
	{
	protected:
		std::atomic<unsigned> m_refs = {0};

	public:
		cow_atomic_base() = default;
		cow_atomic_base(cow_atomic_base &&) = default;
		cow_atomic_base(const cow_atomic_base &) noexcept {};

		cow_atomic_base & operator =(cow_atomic_base &&) = default;
		cow_atomic_base & operator =(const cow_atomic_base &) noexcept { return *this; }


		template <class Derived> friend      void intrusive_ptr_add_ref(cow_atomic_base<Derived> * ptr) noexcept;
		template <class Derived> friend      void intrusive_ptr_release(cow_atomic_base<Derived> * ptr) noexcept;
		template <class Derived> friend  unsigned intrusive_ptr_use_count(cow_atomic_base<Derived> * ptr) noexcept;
		template <class Derived> friend Derived * intrusive_ptr_clone(cow_atomic_base<Derived> * ptr);
	};

	template <class Derived>
	void intrusive_ptr_add_ref(cow_atomic_base<Derived> * ptr) noexcept
	{
		ptr->m_refs.fetch_add(1, std::memory_order_relaxed);
	}

	template <class Derived>
	void intrusive_ptr_release(cow_atomic_base<Derived> * ptr) noexcept
	{ 
		if (ptr->m_refs.fetch_sub(1, std::memory_order_release) == 1)
		{
			std::atomic_thread_fence(std::memory_order_acquire);
			delete static_cast<Derived *>(ptr);
		}
	}

	template <class Derived>
	unsigned intrusive_ptr_use_count(cow_atomic_base<Derived> * ptr) noexcept
	{
		return ptr->m_refs.load(std::memory_order_relaxed);
	}

	template <class Derived>
	Derived * intrusive_ptr_clone(cow_atomic_base<Derived> * ptr)
	{
		auto * copy = new Derived(*static_cast<Derived *>(ptr));
		copy->m_refs.store(1u, std::memory_order_relaxed);
		return copy;
	}



	template <class Derived>
	class cow_plain_base
	{
	protected:
		unsigned m_refs = 0;

	public:
		cow_plain_base() = default;
		cow_plain_base(cow_plain_base &&) = default;
		cow_plain_base(const cow_plain_base &) noexcept {};

		cow_plain_base & operator =(cow_plain_base &&) = default;
		cow_plain_base & operator =(const cow_plain_base &) noexcept { return *this; }


		template <class Derived> friend      void intrusive_ptr_add_ref(cow_plain_base<Derived> * ptr) noexcept;
		template <class Derived> friend      void intrusive_ptr_release(cow_plain_base<Derived> * ptr) noexcept;
		template <class Derived> friend  unsigned intrusive_ptr_use_count(cow_plain_base<Derived> * ptr) noexcept;
		template <class Derived> friend Derived * intrusive_ptr_clone(cow_plain_base<Derived> * ptr);
	};

	template <class Derived>
	void intrusive_ptr_add_ref(cow_plain_base<Derived> * ptr) noexcept
	{
		++ptr->m_refs;
	}

	template <class Derived>
	void intrusive_ptr_release(cow_plain_base<Derived> * ptr) noexcept
	{
		if (--ptr->m_refs == 0)
			delete static_cast<Derived *>(ptr);
	}

	template <class Derived>
	unsigned intrusive_ptr_use_count(cow_plain_base<Derived> * ptr) noexcept
	{
		return ptr->m_refs;
	}

	template <class Derived>
	Derived * intrusive_ptr_clone(cow_plain_base<Derived> * ptr)
	{
		auto * copy = new Derived(*static_cast<Derived *>(ptr));
		copy->m_refs = 1u;
		return copy;
	}



	template <class Type>
	class intrusive_cow_ptr
	{
		typedef intrusive_cow_ptr self_type;

	public:
		typedef Type       value_type;
		typedef Type *     pointer;
		typedef Type &     referecne;

		static_assert(std::is_copy_constructible<Type>::value, "");
		typedef decltype(intrusive_ptr_use_count<Type>(std::declval<Type *>())) count_type;

	private:
		value_type * m_ptr = nullptr;

	public:
		count_type use_count()               const noexcept { return intrusive_ptr_use_count(m_ptr); }
		void reset(value_type * ptr)               noexcept;
		void reset(value_type * ptr, bool add_ref) noexcept;
		void detach();

		inline       value_type * get()       { detach(); return m_ptr; }
		inline const value_type * get() const { return m_ptr; }

		inline       value_type & operator *()       { return *get(); }
		inline const value_type & operator *() const { return *get(); }

		inline       value_type * operator ->()       { return get(); }
		inline const value_type * operator ->() const { return get(); }

	public:
		explicit intrusive_cow_ptr(value_type * ptr)               noexcept : m_ptr(ptr) { intrusive_ptr_add_ref(m_ptr); }
		explicit intrusive_cow_ptr(value_type * ptr, bool add_ref) noexcept : m_ptr(ptr) { if (add_ref) intrusive_ptr_add_ref(m_ptr); }
		
		intrusive_cow_ptr() = default;
		~intrusive_cow_ptr() { intrusive_ptr_release(m_ptr); }

		intrusive_cow_ptr(const self_type & op) noexcept : m_ptr(op.m_ptr) { intrusive_ptr_add_ref(m_ptr); }
		intrusive_cow_ptr(self_type && op)      noexcept : m_ptr(std::exchange(op.m_ptr, nullptr)) {}

		intrusive_cow_ptr & operator =(const self_type & op) noexcept { if (this != &op) reset(op.m_ptr);                          return *this; }
		intrusive_cow_ptr & operator =(self_type && op)      noexcept { if (this != &op) m_ptr = std::exchange(op.m_ptr, nullptr); return *this; }

		friend void swap(intrusive_cow_ptr & p1, intrusive_cow_ptr & p2) { std::swap(p1.m_ptr, p2.m_ptr); }
	};


	template <class Type>
	inline void intrusive_cow_ptr<Type>::detach()
	{
		// we are the only one, detach is not needed
		if (intrusive_ptr_use_count(m_ptr) <= 1) return;

		// make a copy
		auto * copy = intrusive_ptr_clone(m_ptr);
		intrusive_ptr_release(m_ptr);
		m_ptr = copy;
	}

	template <class Type>
	inline void intrusive_cow_ptr<Type>::reset(value_type * ptr) noexcept
	{
		intrusive_ptr_release(m_ptr);
		intrusive_ptr_add_ref(ptr);
		m_ptr = ptr;
	}

	template <class Type>
	inline void intrusive_cow_ptr<Type>::reset(value_type * ptr, bool add_ref) noexcept
	{
		intrusive_ptr_release(m_ptr);
		if (add_ref) intrusive_ptr_add_ref(ptr);
		m_ptr = ptr;
	}
}
