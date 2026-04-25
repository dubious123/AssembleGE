#include "age_test_pch.hpp"
#include "age_test.hpp"

namespace age_test::data_structure::sparse_vector
{
	// ============================================================
	// non-trivial value type for destroy/copy verification
	// ============================================================
	struct tracked
	{
		static inline int32 alive_count = 0;

		uint32 value = 0;

		tracked() noexcept { ++alive_count; }

		tracked(uint32 v) noexcept : value{ v } { ++alive_count; }

		tracked(const tracked& other) noexcept : value{ other.value } { ++alive_count; }

		tracked(tracked&& other) noexcept : value{ other.value } { ++alive_count; }

		~tracked() noexcept { --alive_count; }

		tracked&
		operator=(const tracked& other) noexcept
		{
			value = other.value;
			return *this;
		}

		tracked&
		operator=(tracked&& other) noexcept
		{
			value = other.value;
			return *this;
		}

		bool
		operator==(const tracked& other) const noexcept
		{ return value == other.value; }
	};

	// non-trivial movable, not trivially copyable
	struct movable_only
	{
		age::vector<uint32> data;

		movable_only() = default;

		movable_only(uint32 v) : data{} { data.emplace_back(v); }

		movable_only(const movable_only&)	  = default;
		movable_only(movable_only&&) noexcept = default;
		movable_only&
		operator=(const movable_only&) = default;
		movable_only&
		operator=(movable_only&&) noexcept = default;

		uint32
		value() const noexcept
		{ return data.empty() ? 0 : data[0]; }
	};

	// ============================================================
	// 1. default constructor
	// ============================================================
	void
	test_default_constructor()
	{
		age::sparse_vector<uint32> sv;

		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.capacity() == 0);
		AGE_ASSERT(sv.empty());
		AGE_ASSERT(sv.is_empty());
		AGE_ASSERT(sv.hole_count() == 0);
		AGE_ASSERT(sv.begin() == sv.end());
	}

	// ============================================================
	// 2. gen_reserve
	// ============================================================
	void
	test_gen_reserve()
	{
		auto sv = age::sparse_vector<uint32>::gen_reserve(64);

		AGE_ASSERT(sv.capacity() == 64);
		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.hole_count() == 64);
		AGE_ASSERT(sv.empty());
		AGE_ASSERT(sv.begin() == sv.end());
	}

	// ============================================================
	// 3. emplace_back single
	// ============================================================
	void
	test_emplace_back_single()
	{
		age::sparse_vector<uint32> sv;

		c_auto idx = sv.emplace_back(42);

		AGE_ASSERT(idx == 0);
		AGE_ASSERT(sv.size() == 1);
		AGE_ASSERT(sv[0] == 42);
		AGE_ASSERT(sv.is_live(0));
	}

	// ============================================================
	// 4. emplace_back returns sequential idx
	// ============================================================
	void
	test_emplace_back_sequential()
	{
		age::sparse_vector<uint32> sv;

		for (uint32 i = 0; i < 100; ++i)
		{
			c_auto idx = sv.emplace_back(i * 10);
			AGE_ASSERT(idx == i);
		}

		AGE_ASSERT(sv.size() == 100);

		for (uint32 i = 0; i < 100; ++i)
		{
			AGE_ASSERT(sv[i] == i * 10);
			AGE_ASSERT(sv.is_live(i));
		}
	}

	// ============================================================
	// 5. emplace_back triggers grow
	// ============================================================
	void
	test_emplace_back_grow()
	{
		age::sparse_vector<uint32> sv;

		c_auto cap_at_zero = sv.capacity();
		AGE_ASSERT(cap_at_zero == 0);

		for (uint32 i = 0; i < 1000; ++i)
		{
			sv.emplace_back(i);
		}

		AGE_ASSERT(sv.size() == 1000);
		AGE_ASSERT(sv.capacity() >= 1000);

		for (uint32 i = 0; i < 1000; ++i)
		{
			AGE_ASSERT(sv[i] == i);
		}
	}

	// ============================================================
	// 6. remove single
	// ============================================================
	void
	test_remove_single()
	{
		age::sparse_vector<uint32> sv;

		c_auto idx = sv.emplace_back(42);
		AGE_ASSERT(sv.size() == 1);

		sv.remove(idx);
		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.is_live(0) is_false);
	}

	// ============================================================
	// 7. remove reuses hole (LIFO)
	// ============================================================
	void
	test_remove_reuses_hole()
	{
		age::sparse_vector<uint32> sv;

		c_auto idx0 = sv.emplace_back(10);
		c_auto idx1 = sv.emplace_back(20);
		c_auto idx2 = sv.emplace_back(30);
		(void)idx0;
		(void)idx2;

		sv.remove(idx1);
		AGE_ASSERT(sv.is_live(idx1) is_false);

		c_auto reused = sv.emplace_back(99);
		AGE_ASSERT(reused == idx1);
		AGE_ASSERT(sv[idx1] == 99);
	}

	// ============================================================
	// 8. remove LIFO order
	// ============================================================
	void
	test_remove_lifo_order()
	{
		age::sparse_vector<uint32> sv;

		c_auto a = sv.emplace_back(1);
		c_auto b = sv.emplace_back(2);
		c_auto c = sv.emplace_back(3);

		sv.remove(a);
		sv.remove(b);
		sv.remove(c);

		// LIFO: c was last freed, comes first
		AGE_ASSERT(sv.emplace_back(11) == c);
		AGE_ASSERT(sv.emplace_back(22) == b);
		AGE_ASSERT(sv.emplace_back(33) == a);
	}

	// ============================================================
	// 9. operator[]
	// ============================================================
	void
	test_operator_bracket()
	{
		age::sparse_vector<uint32> sv;
		c_auto					   idx = sv.emplace_back(42);

		sv[idx] = 99;
		AGE_ASSERT(sv[idx] == 99);

		c_auto& csv = sv;
		AGE_ASSERT(csv[idx] == 99);
	}

	// ============================================================
	// 10. operator[] with handle
	// ============================================================
	void
	test_operator_bracket_handle()
	{
		struct fake_handle
		{
			std::size_t id;
		};

		age::sparse_vector<uint32> sv;
		c_auto					   idx = sv.emplace_back(123);

		fake_handle h{ idx };
		AGE_ASSERT(sv[h] == 123);
	}

	// ============================================================
	// 11. remove with handle
	// ============================================================
	void
	test_remove_handle()
	{
		struct fake_handle
		{
			std::size_t id;
		};

		age::sparse_vector<uint32> sv;
		c_auto					   idx = sv.emplace_back(42);

		fake_handle h{ idx };
		sv.remove(h);

		AGE_ASSERT(sv.size() == 0);
	}

	// ============================================================
	// 12. front / back
	// ============================================================
	void
	test_front_back()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(10);
		sv.emplace_back(20);
		sv.emplace_back(30);

		AGE_ASSERT(sv.front() == 10);
		AGE_ASSERT(sv.back() == 30);

		// remove front, back stays the same
		sv.remove(0);
		AGE_ASSERT(sv.front() == 20);
		AGE_ASSERT(sv.back() == 30);
	}

	// ============================================================
	// 13. front / back across holes
	// ============================================================
	void
	test_front_back_with_holes()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		// remove first 3 and last 3
		sv.remove(0);
		sv.remove(1);
		sv.remove(2);
		sv.remove(7);
		sv.remove(8);
		sv.remove(9);

		AGE_ASSERT(sv.front() == 3);
		AGE_ASSERT(sv.back() == 6);
	}

	// ============================================================
	// 14. clear preserves capacity
	// ============================================================
	void
	test_clear()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 100; ++i)
		{
			sv.emplace_back(i);
		}

		c_auto cap_before = sv.capacity();
		sv.clear();

		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.capacity() == cap_before);
		AGE_ASSERT(sv.empty());
		AGE_ASSERT(sv.hole_count() == cap_before);

		// new emplace starts from idx 0
		c_auto idx = sv.emplace_back(99);
		AGE_ASSERT(idx == 0);
	}

	// ============================================================
	// 15. reserve
	// ============================================================
	void
	test_reserve()
	{
		age::sparse_vector<uint32> sv;
		sv.reserve(128);

		AGE_ASSERT(sv.capacity() >= 128);
		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.hole_count() == sv.capacity());
	}

	// ============================================================
	// 16. reserve preserves data
	// ============================================================
	void
	test_reserve_preserves_data()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 50; ++i)
		{
			sv.emplace_back(i * 7);
		}

		sv.reserve(1000);
		AGE_ASSERT(sv.capacity() >= 1000);
		AGE_ASSERT(sv.size() == 50);

		for (uint32 i = 0; i < 50; ++i)
		{
			AGE_ASSERT(sv[i] == i * 7);
		}
	}

	// ============================================================
	// 17. reserve preserves holes
	// ============================================================
	void
	test_reserve_preserves_holes()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 20; ++i)
		{
			sv.emplace_back(i);
		}

		sv.remove(5);
		sv.remove(10);
		sv.remove(15);

		sv.reserve(2000);

		AGE_ASSERT(sv.size() == 17);
		AGE_ASSERT(sv.is_live(5) is_false);
		AGE_ASSERT(sv.is_live(10) is_false);
		AGE_ASSERT(sv.is_live(15) is_false);

		// idx not removed should stay live
		for (uint32 i = 0; i < 20; ++i)
		{
			if (i == 5 || i == 10 || i == 15) { continue; }
			AGE_ASSERT(sv[i] == i);
		}

		c_auto cap_after_reserve = sv.capacity();
		AGE_ASSERT(cap_after_reserve >= 2000);
		AGE_ASSERT(sv.hole_count() == cap_after_reserve - 17);

		auto seen = age::vector<bool>{};

		seen.resize(cap_after_reserve, false);


		for (uint32 i = 0; i < 20; ++i)
		{
			if (i == 5 || i == 10 || i == 15) { continue; }
			seen[i] = true;
		}

		c_auto holes_to_fill = sv.hole_count();
		for (std::size_t i = 0; i < holes_to_fill; ++i)
		{
			c_auto idx = sv.emplace_back(static_cast<uint32>(1000 + i));

			AGE_ASSERT(idx < cap_after_reserve);
			AGE_ASSERT(seen[idx] is_false);
			seen[idx] = true;
		}

		AGE_ASSERT(sv.size() == cap_after_reserve);
		AGE_ASSERT(sv.hole_count() == 0);

		for (std::size_t i = 0; i < cap_after_reserve; ++i)
		{
			AGE_ASSERT(seen[i]);
			AGE_ASSERT(sv.is_live(i));
		}

		AGE_ASSERT(sv.is_live(5));
		AGE_ASSERT(sv.is_live(10));
		AGE_ASSERT(sv.is_live(15));
	}

	// ============================================================
	// 18. is_live
	// ============================================================
	void
	test_is_live()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(1);
		sv.emplace_back(2);
		sv.emplace_back(3);

		AGE_ASSERT(sv.is_live(0));
		AGE_ASSERT(sv.is_live(1));
		AGE_ASSERT(sv.is_live(2));

		sv.remove(1);
		AGE_ASSERT(sv.is_live(1) is_false);
	}

	// ============================================================
	// 19. hole_count
	// ============================================================
	void
	test_hole_count()
	{
		auto sv = age::sparse_vector<uint32>::gen_reserve(10);
		AGE_ASSERT(sv.hole_count() == 10);

		sv.emplace_back(1);
		sv.emplace_back(2);
		AGE_ASSERT(sv.hole_count() == 8);

		sv.remove(0);
		AGE_ASSERT(sv.hole_count() == 9);
	}

	// ============================================================
	// 20. iterator forward - basic
	// ============================================================
	void
	test_iterator_basic()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(10);
		sv.emplace_back(20);
		sv.emplace_back(30);

		uint32 sum	 = 0;
		uint32 count = 0;
		for (auto v : sv)
		{
			sum += v;
			++count;
		}

		AGE_ASSERT(count == 3);
		AGE_ASSERT(sum == 60);
	}

	// ============================================================
	// 21. iterator skips holes
	// ============================================================
	void
	test_iterator_skips_holes()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		sv.remove(2);
		sv.remove(5);
		sv.remove(7);

		auto values = age::vector<uint32>{};
		for (auto v : sv)
		{
			values.emplace_back(v);
		}

		AGE_ASSERT(values.size() == 7);
		AGE_ASSERT(values[0] == 0);
		AGE_ASSERT(values[1] == 1);
		AGE_ASSERT(values[2] == 3);
		AGE_ASSERT(values[3] == 4);
		AGE_ASSERT(values[4] == 6);
		AGE_ASSERT(values[5] == 8);
		AGE_ASSERT(values[6] == 9);
	}

	// ============================================================
	// 22. iterator empty
	// ============================================================
	void
	test_iterator_empty()
	{
		age::sparse_vector<uint32> sv;
		AGE_ASSERT(sv.begin() == sv.end());

		sv.emplace_back(1);
		sv.remove(0);
		AGE_ASSERT(sv.begin() == sv.end());
	}

	// ============================================================
	// 23. const_iterator
	// ============================================================
	void
	test_const_iterator()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 5; ++i)
		{
			sv.emplace_back(i);
		}

		c_auto& csv = sv;
		uint32	sum = 0;
		for (auto v : csv)
		{
			sum += v;
		}
		AGE_ASSERT(sum == 0 + 1 + 2 + 3 + 4);

		auto   cit	 = csv.cbegin();
		auto   cend	 = csv.cend();
		uint32 count = 0;
		for (; cit != cend; ++cit)
		{
			++count;
		}
		AGE_ASSERT(count == 5);
	}

	// ============================================================
	// 24. iterator -> const_iterator conversion
	// ============================================================
	void
	test_iterator_const_conversion()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(42);

		auto									   it  = sv.begin();
		age::sparse_vector<uint32>::const_iterator cit = it;	// implicit conversion
		AGE_ASSERT(*cit == 42);
	}

	// ============================================================
	// 25. iterator idx() accessor
	// ============================================================
	void
	test_iterator_idx()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 5; ++i)
		{
			sv.emplace_back(i);
		}

		sv.remove(1);
		sv.remove(3);

		auto values = age::vector<std::size_t>{};
		for (auto it = sv.begin(); it != sv.end(); ++it)
		{
			values.emplace_back(it.idx());
		}

		AGE_ASSERT(values.size() == 3);
		AGE_ASSERT(values[0] == 0);
		AGE_ASSERT(values[1] == 2);
		AGE_ASSERT(values[2] == 4);
	}

	// ============================================================
	// 26. reverse iterator basic
	// ============================================================
	void
	test_reverse_iterator_basic()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(10);
		sv.emplace_back(20);
		sv.emplace_back(30);

		auto values = age::vector<uint32>{};
		for (auto it = sv.rbegin(); it != sv.rend(); ++it)
		{
			values.emplace_back(*it);
		}

		AGE_ASSERT(values.size() == 3);
		AGE_ASSERT(values[0] == 30);
		AGE_ASSERT(values[1] == 20);
		AGE_ASSERT(values[2] == 10);
	}

	// ============================================================
	// 27. reverse iterator skips holes
	// ============================================================
	void
	test_reverse_iterator_skips_holes()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		sv.remove(2);
		sv.remove(5);
		sv.remove(8);

		auto values = age::vector<uint32>{};
		for (auto it = sv.rbegin(); it != sv.rend(); ++it)
		{
			values.emplace_back(*it);
		}

		AGE_ASSERT(values.size() == 7);
		AGE_ASSERT(values[0] == 9);
		AGE_ASSERT(values[1] == 7);
		AGE_ASSERT(values[2] == 6);
		AGE_ASSERT(values[3] == 4);
		AGE_ASSERT(values[4] == 3);
		AGE_ASSERT(values[5] == 1);
		AGE_ASSERT(values[6] == 0);
	}

	// ============================================================
	// 28. reverse iterator empty
	// ============================================================
	void
	test_reverse_iterator_empty()
	{
		age::sparse_vector<uint32> sv;
		AGE_ASSERT(sv.rbegin() == sv.rend());
	}

	// ============================================================
	// 29. const reverse iterator
	// ============================================================
	void
	test_const_reverse_iterator()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 4; ++i)
		{
			sv.emplace_back(i);
		}

		c_auto& csv	   = sv;
		auto	values = age::vector<uint32>{};
		for (auto it = csv.crbegin(); it != csv.crend(); ++it)
		{
			values.emplace_back(*it);
		}

		AGE_ASSERT(values.size() == 4);
		AGE_ASSERT(values[0] == 3);
		AGE_ASSERT(values[1] == 2);
		AGE_ASSERT(values[2] == 1);
		AGE_ASSERT(values[3] == 0);
	}

	// ============================================================
	// 30. iterator post-increment
	// ============================================================
	void
	test_iterator_post_increment()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(10);
		sv.emplace_back(20);

		auto it		= sv.begin();
		auto old_it = it++;

		AGE_ASSERT(*old_it == 10);
		AGE_ASSERT(*it == 20);
	}

	// ============================================================
	// 31. copy constructor
	// ============================================================
	void
	test_copy_constructor()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 20; ++i)
		{
			sv.emplace_back(i);
		}
		sv.remove(5);
		sv.remove(10);

		auto copy = sv;

		AGE_ASSERT(copy.size() == sv.size());
		AGE_ASSERT(copy.capacity() == sv.capacity());

		for (uint32 i = 0; i < 20; ++i)
		{
			if (i == 5 || i == 10)
			{
				AGE_ASSERT(copy.is_live(i) is_false);
			}
			else
			{
				AGE_ASSERT(copy.is_live(i));
				AGE_ASSERT(copy[i] == i);
			}
		}
	}

	// ============================================================
	// 32. copy is independent
	// ============================================================
	void
	test_copy_independence()
	{
		age::sparse_vector<uint32> sv;
		c_auto					   idx = sv.emplace_back(42);

		auto copy = sv;
		copy[idx] = 999;

		AGE_ASSERT(sv[idx] == 42);
		AGE_ASSERT(copy[idx] == 999);
	}

	// ============================================================
	// 33. move constructor
	// ============================================================
	void
	test_move_constructor()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 50; ++i)
		{
			sv.emplace_back(i);
		}

		auto moved = std::move(sv);

		AGE_ASSERT(moved.size() == 50);
		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.capacity() == 0);

		for (uint32 i = 0; i < 50; ++i)
		{
			AGE_ASSERT(moved[i] == i);
		}
	}

	// ============================================================
	// 34. copy assignment
	// ============================================================
	void
	test_copy_assignment()
	{
		age::sparse_vector<uint32> src;
		for (uint32 i = 0; i < 30; ++i)
		{
			src.emplace_back(i * 2);
		}

		age::sparse_vector<uint32> dst;
		dst.emplace_back(999);
		dst.emplace_back(888);

		dst = src;

		AGE_ASSERT(dst.size() == src.size());
		for (uint32 i = 0; i < 30; ++i)
		{
			AGE_ASSERT(dst[i] == i * 2);
		}
	}

	// ============================================================
	// 35. move assignment
	// ============================================================
	void
	test_move_assignment()
	{
		age::sparse_vector<uint32> src;
		for (uint32 i = 0; i < 30; ++i)
		{
			src.emplace_back(i);
		}

		age::sparse_vector<uint32> dst;
		dst.emplace_back(999);

		dst = std::move(src);

		AGE_ASSERT(dst.size() == 30);
		AGE_ASSERT(src.size() == 0);

		for (uint32 i = 0; i < 30; ++i)
		{
			AGE_ASSERT(dst[i] == i);
		}
	}

	// ============================================================
	// 36. self copy assignment
	// ============================================================
	void
	test_self_copy_assignment()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		sv = sv;

		AGE_ASSERT(sv.size() == 10);
		for (uint32 i = 0; i < 10; ++i)
		{
			AGE_ASSERT(sv[i] == i);
		}
	}

	// ============================================================
	// 37. self move assignment
	// ============================================================
	void
	test_self_move_assignment()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		sv = std::move(sv);

		AGE_ASSERT(sv.size() == 10);
		for (uint32 i = 0; i < 10; ++i)
		{
			AGE_ASSERT(sv[i] == i);
		}
	}

	// ============================================================
	// 38. non-trivial type - destruction count
	// ============================================================
	void
	test_non_trivial_destruction()
	{
		tracked::alive_count = 0;

		{
			age::sparse_vector<tracked> sv;
			for (uint32 i = 0; i < 100; ++i)
			{
				sv.emplace_back(i);
			}
			AGE_ASSERT(tracked::alive_count == 100);

			for (uint32 i = 0; i < 50; ++i)
			{
				sv.remove(i);
			}
			AGE_ASSERT(tracked::alive_count == 50);
		}

		AGE_ASSERT(tracked::alive_count == 0);
	}

	// ============================================================
	// 39. non-trivial type - clear destroys all
	// ============================================================
	void
	test_non_trivial_clear()
	{
		tracked::alive_count = 0;

		age::sparse_vector<tracked> sv;
		for (uint32 i = 0; i < 50; ++i)
		{
			sv.emplace_back(i);
		}
		AGE_ASSERT(tracked::alive_count == 50);

		sv.clear();
		AGE_ASSERT(tracked::alive_count == 0);
		AGE_ASSERT(sv.size() == 0);
	}

	// ============================================================
	// 40. non-trivial type - copy ctor
	// ============================================================
	void
	test_non_trivial_copy()
	{
		tracked::alive_count = 0;

		{
			age::sparse_vector<tracked> sv;
			for (uint32 i = 0; i < 30; ++i)
			{
				sv.emplace_back(i);
			}

			auto copy = sv;
			AGE_ASSERT(tracked::alive_count == 60);

			for (uint32 i = 0; i < 30; ++i)
			{
				AGE_ASSERT(copy[i].value == i);
			}
		}

		AGE_ASSERT(tracked::alive_count == 0);
	}

	// ============================================================
	// 41. non-trivial type - grow preserves elements
	// ============================================================
	void
	test_non_trivial_grow()
	{
		tracked::alive_count = 0;

		{
			age::sparse_vector<tracked> sv;
			for (uint32 i = 0; i < 500; ++i)
			{
				sv.emplace_back(i);
			}

			AGE_ASSERT(tracked::alive_count == 500);
			for (uint32 i = 0; i < 500; ++i)
			{
				AGE_ASSERT(sv[i].value == i);
			}
		}

		AGE_ASSERT(tracked::alive_count == 0);
	}

	// ============================================================
	// 42. non-trivial type - relocate after grow with holes
	// ============================================================
	void
	test_non_trivial_relocate_with_holes()
	{
		tracked::alive_count = 0;

		{
			auto sv = age::sparse_vector<tracked>::gen_reserve(8);
			for (uint32 i = 0; i < 8; ++i)
			{
				sv.emplace_back(i * 10);
			}

			sv.remove(2);
			sv.remove(5);
			AGE_ASSERT(tracked::alive_count == 6);

			// trigger grow
			sv.reserve(1000);
			AGE_ASSERT(tracked::alive_count == 6);

			for (uint32 i = 0; i < 8; ++i)
			{
				if (i == 2 || i == 5) { continue; }
				AGE_ASSERT(sv[i].value == i * 10);
			}
		}

		AGE_ASSERT(tracked::alive_count == 0);
	}

	// ============================================================
	// 43. movable_only type
	// ============================================================
	void
	test_movable_only()
	{
		age::sparse_vector<movable_only> sv;
		for (uint32 i = 0; i < 100; ++i)
		{
			sv.emplace_back(i);
		}

		for (uint32 i = 0; i < 100; ++i)
		{
			AGE_ASSERT(sv[i].value() == i);
		}

		// trigger grow with non-trivial relocation
		for (uint32 i = 100; i < 1000; ++i)
		{
			sv.emplace_back(i);
		}

		for (uint32 i = 0; i < 1000; ++i)
		{
			AGE_ASSERT(sv[i].value() == i);
		}
	}

	// ============================================================
	// 44. boundary - cap exactly 64
	// ============================================================
	void
	test_boundary_cap_64()
	{
		auto sv = age::sparse_vector<uint32>::gen_reserve(64);
		for (uint32 i = 0; i < 64; ++i)
		{
			sv.emplace_back(i);
		}

		AGE_ASSERT(sv.size() == 64);
		AGE_ASSERT(sv.hole_count() == 0);

		uint32 count = 0;
		for (auto v : sv)
		{
			(void)v;
			++count;
		}
		AGE_ASSERT(count == 64);
	}

	// ============================================================
	// 45. boundary - cap 65 (crosses word boundary)
	// ============================================================
	void
	test_boundary_cap_65()
	{
		auto sv = age::sparse_vector<uint32>::gen_reserve(65);
		for (uint32 i = 0; i < 65; ++i)
		{
			sv.emplace_back(i);
		}

		AGE_ASSERT(sv.size() == 65);

		uint32 count = 0;
		uint64 sum	 = 0;
		for (auto v : sv)
		{
			sum += v;
			++count;
		}
		AGE_ASSERT(count == 65);
		AGE_ASSERT(sum == 64ull * 65 / 2);
	}

	// ============================================================
	// 46. boundary - cap 127, 128, 129
	// ============================================================
	void
	test_boundary_caps()
	{
		for (auto cap : { std::size_t{ 127 }, std::size_t{ 128 }, std::size_t{ 129 } })
		{
			auto sv = age::sparse_vector<uint32>::gen_reserve(cap);
			for (uint32 i = 0; i < cap; ++i)
			{
				sv.emplace_back(i);
			}

			AGE_ASSERT(sv.size() == cap);

			uint32 count = 0;
			for (auto v : sv)
			{
				(void)v;
				++count;
			}
			AGE_ASSERT(count == cap);

			// reverse
			count = 0;
			for (auto it = sv.rbegin(); it != sv.rend(); ++it)
			{
				++count;
			}
			AGE_ASSERT(count == cap);
		}
	}

	// ============================================================
	// 47. front_idx / back_idx via front()/back() across word boundary
	// ============================================================
	void
	test_word_boundary_front_back()
	{
		auto sv = age::sparse_vector<uint32>::gen_reserve(128);
		for (uint32 i = 0; i < 128; ++i)
		{
			sv.emplace_back(i);
		}

		// remove all in word 0 except idx 63
		for (uint32 i = 0; i < 63; ++i)
		{
			sv.remove(i);
		}
		// remove all in word 1 except idx 64
		for (uint32 i = 65; i < 128; ++i)
		{
			sv.remove(i);
		}

		AGE_ASSERT(sv.front() == 63);
		AGE_ASSERT(sv.back() == 64);
	}

	// ============================================================
	// 48. emplace after full removal
	// ============================================================
	void
	test_emplace_after_full_removal()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 50; ++i)
		{
			sv.emplace_back(i);
		}
		for (uint32 i = 0; i < 50; ++i)
		{
			sv.remove(i);
		}

		AGE_ASSERT(sv.size() == 0);

		c_auto cap = sv.capacity();
		// re-fill - should not grow
		for (uint32 i = 0; i < 50; ++i)
		{
			sv.emplace_back(100 + i);
		}
		AGE_ASSERT(sv.capacity() == cap);
		AGE_ASSERT(sv.size() == 50);
	}

	// ============================================================
	// 49. interleaved emplace/remove
	// ============================================================
	void
	test_interleaved_emplace_remove()
	{
		age::sparse_vector<uint32> sv;

		auto live_idxs = age::vector<std::size_t>{};
		for (uint32 round = 0; round < 100; ++round)
		{
			c_auto idx = sv.emplace_back(round);
			live_idxs.emplace_back(idx);

			if (round % 3 == 0 && live_idxs.empty() is_false)
			{
				c_auto victim_pos = round % live_idxs.size();
				c_auto victim	  = live_idxs[victim_pos];
				sv.remove(victim);
				live_idxs[victim_pos] = live_idxs.back();
				live_idxs.resize(live_idxs.size() - 1);
			}
		}

		// iterate must not crash; count must match live_idxs
		uint32 iter_count = 0;
		for (auto v : sv)
		{
			(void)v;
			++iter_count;
		}
		AGE_ASSERT(iter_count == live_idxs.size());
		AGE_ASSERT(sv.size() == live_idxs.size());
	}

	// ============================================================
	// 50. stress - 50000 sequential emplace
	// ============================================================
	void
	test_stress_sequential()
	{
		age::sparse_vector<uint32> sv;

		for (uint32 i = 0; i < 50000; ++i)
		{
			c_auto idx = sv.emplace_back(i);
			AGE_ASSERT(idx == i);
		}

		AGE_ASSERT(sv.size() == 50000);

		uint32 count = 0;
		for (auto v : sv)
		{
			AGE_ASSERT(v == count);
			++count;
		}
		AGE_ASSERT(count == 50000);
	}

	// ============================================================
	// 51. stress - emplace + remove cycles
	// ============================================================
	void
	test_stress_emplace_remove_cycles()
	{
		age::sparse_vector<uint32> sv;

		for (uint32 cycle = 0; cycle < 100; ++cycle)
		{
			auto idxs = age::vector<std::size_t>{};
			for (uint32 i = 0; i < 200; ++i)
			{
				idxs.emplace_back(sv.emplace_back(cycle * 1000 + i));
			}

			for (auto idx : idxs)
			{
				sv.remove(idx);
			}
			AGE_ASSERT(sv.size() == 0);
		}

		// verify final state
		AGE_ASSERT(sv.size() == 0);
		c_auto idx = sv.emplace_back(42);
		AGE_ASSERT(sv[idx] == 42);
	}

	// ============================================================
	// 52. stress - random ops
	// ============================================================
	void
	test_stress_random_ops()
	{
		age::sparse_vector<uint32> sv;
		auto					   live = age::vector<std::size_t>{};

		uint32 seed		 = 0xDEADBEEF;
		auto   next_rand = [&]() -> uint32 {
			seed = seed * 1103515245u + 12345u;
			return seed;
		};

		for (uint32 i = 0; i < 20000; ++i)
		{
			c_auto op = next_rand() % 3;

			if (op == 0 || live.empty())
			{
				c_auto idx = sv.emplace_back(i);
				live.emplace_back(idx);
			}
			else if (op == 1)
			{
				c_auto pos = next_rand() % live.size();
				sv.remove(live[pos]);
				live[pos] = live.back();
				live.resize(live.size() - 1);
			}
			else
			{
				// iteration
				uint32 c = 0;
				for (auto v : sv)
				{
					(void)v;
					++c;
				}
				AGE_ASSERT(c == live.size());
			}
		}

		AGE_ASSERT(sv.size() == live.size());
	}

	// ============================================================
	// 53. stress - copy under load
	// ============================================================
	void
	test_stress_copy_under_load()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 5000; ++i)
		{
			sv.emplace_back(i);
		}

		// punch holes
		for (uint32 i = 0; i < 5000; i += 7)
		{
			sv.remove(i);
		}

		auto copy = sv;
		AGE_ASSERT(copy.size() == sv.size());

		uint32 sv_sum = 0, copy_sum = 0;
		for (auto v : sv)
		{
			sv_sum += v;
		}
		for (auto v : copy)
		{
			copy_sum += v;
		}
		AGE_ASSERT(sv_sum == copy_sum);
	}

	// ============================================================
	// 54. stress - non-trivial type lifecycle
	// ============================================================
	void
	test_stress_non_trivial_lifecycle()
	{
		tracked::alive_count = 0;

		{
			age::sparse_vector<tracked> sv;

			// add 2000
			auto idxs = age::vector<std::size_t>{};
			for (uint32 i = 0; i < 2000; ++i)
			{
				idxs.emplace_back(sv.emplace_back(i));
			}
			AGE_ASSERT(tracked::alive_count == 2000);

			// remove half
			for (uint32 i = 0; i < 1000; ++i)
			{
				sv.remove(idxs[i]);
			}
			AGE_ASSERT(tracked::alive_count == 1000);

			// re-add 500
			for (uint32 i = 0; i < 500; ++i)
			{
				sv.emplace_back(10000 + i);
			}
			AGE_ASSERT(tracked::alive_count == 1500);

			// copy
			auto copy = sv;
			AGE_ASSERT(tracked::alive_count == 3000);

			// reserve to force grow
			sv.reserve(10000);
			AGE_ASSERT(tracked::alive_count == 3000);

			// clear copy
			copy.clear();
			AGE_ASSERT(tracked::alive_count == 1500);
		}

		AGE_ASSERT(tracked::alive_count == 0);
	}

	// ============================================================
	// 55. stress - large grow chain
	// ============================================================
	void
	test_stress_grow_chain()
	{
		age::sparse_vector<uint32> sv;

		for (uint32 i = 0; i < 100000; ++i)
		{
			sv.emplace_back(i);
		}

		AGE_ASSERT(sv.size() == 100000);
		AGE_ASSERT(sv[0] == 0);
		AGE_ASSERT(sv[50000] == 50000);
		AGE_ASSERT(sv[99999] == 99999);
	}

	// ============================================================
	// 56. iterator satisfies forward_iterator
	// ============================================================
	void
	test_iterator_concept()
	{
		using sv_t = age::sparse_vector<uint32>;
		static_assert(std::forward_iterator<sv_t::iterator>);
		static_assert(std::forward_iterator<sv_t::const_iterator>);
		static_assert(std::forward_iterator<sv_t::reverse_iterator>);
		static_assert(std::forward_iterator<sv_t::const_reverse_iterator>);
	}

	// ============================================================
	// 57. range concepts
	// ============================================================
	void
	test_range_concept()
	{
		using sv_t = age::sparse_vector<uint32>;
		static_assert(std::ranges::range<sv_t>);
		static_assert(std::ranges::input_range<sv_t>);
		static_assert(std::ranges::forward_range<sv_t>);
		static_assert(std::ranges::range<const sv_t>);
	}

	// ============================================================
	// 58. ranges::find
	// ============================================================
	void
	test_ranges_find()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i * 10);
		}
		sv.remove(3);
		sv.remove(7);

		auto it = std::ranges::find(sv, 50u);
		AGE_ASSERT(it != sv.end());
		AGE_ASSERT(*it == 50);
		AGE_ASSERT(it.idx() == 5);

		auto miss = std::ranges::find(sv, 999u);
		AGE_ASSERT(miss == sv.end());

		auto removed = std::ranges::find(sv, 30u);	  // idx 3 -> removed
		AGE_ASSERT(removed == sv.end());
	}

	// ============================================================
	// 59. ranges::count_if / any_of / all_of
	// ============================================================
	void
	test_ranges_predicates()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 20; ++i)
		{
			sv.emplace_back(i);
		}
		sv.remove(5);
		sv.remove(10);

		c_auto evens = std::ranges::count_if(sv, [](uint32 v) { return v % 2 == 0; });
		AGE_ASSERT(evens == 9);	   // 0,2,4,6,8,12,14,16,18 (10 removed)

		AGE_ASSERT(std::ranges::any_of(sv, [](uint32 v) { return v == 7; }));
		AGE_ASSERT(std::ranges::any_of(sv, [](uint32 v) { return v == 5; }) is_false);

		AGE_ASSERT(std::ranges::all_of(sv, [](uint32 v) { return v < 100; }));
	}

	// ============================================================
	// 60. ranges::for_each + std::accumulate
	// ============================================================
	void
	test_ranges_for_each()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 1; i <= 10; ++i)
		{
			sv.emplace_back(i);
		}
		sv.remove(0);	 // remove value=1 at idx 0
		sv.remove(4);	 // remove value=5 at idx 4

		uint32 sum = 0;
		std::ranges::for_each(sv, [&](uint32 v) { sum += v; });
		AGE_ASSERT(sum == (2 + 3 + 4 + 6 + 7 + 8 + 9 + 10));
	}

	// ============================================================
	// 61. views::filter
	// ============================================================
	void
	test_views_filter()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 20; ++i)
		{
			sv.emplace_back(i);
		}

		auto evens = sv | std::views::filter([](uint32 v) { return v % 2 == 0; });

		uint32 count = 0;
		uint32 sum	 = 0;
		for (auto v : evens)
		{
			++count;
			sum += v;
		}
		AGE_ASSERT(count == 10);
		AGE_ASSERT(sum == 0 + 2 + 4 + 6 + 8 + 10 + 12 + 14 + 16 + 18);
	}

	// ============================================================
	// 62. views::transform
	// ============================================================
	void
	test_views_transform()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 1; i <= 5; ++i)
		{
			sv.emplace_back(i);
		}

		auto squared = sv | std::views::transform([](uint32 v) { return v * v; });

		uint64 sum = 0;
		for (auto v : squared)
		{
			sum += v;
		}
		AGE_ASSERT(sum == 1 + 4 + 9 + 16 + 25);
	}

	// ============================================================
	// 63. mutation through iterator (non-const ranges)
	// ============================================================
	void
	test_iterator_mutation()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		std::ranges::for_each(sv, [](uint32& v) { v *= 10; });

		for (uint32 i = 0; i < 10; ++i)
		{
			AGE_ASSERT(sv[i] == i * 10);
		}
	}

	// ============================================================
	// 64. iterator in std::vector copy
	// ============================================================
	void
	test_iterator_copy_to_vector()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i * 7);
		}
		sv.remove(2);
		sv.remove(5);

		auto vec = age::vector<uint32>{};
		for (auto v : sv)
		{
			vec.emplace_back(v);
		}

		AGE_ASSERT(vec.size() == 8);
		AGE_ASSERT(vec[0] == 0);
		AGE_ASSERT(vec[1] == 7);
		AGE_ASSERT(vec[2] == 21);	 // idx 3
		AGE_ASSERT(vec[7] == 63);	 // idx 9
	}

	// ============================================================
	// 65. iterator equality / inequality
	// ============================================================
	void
	test_iterator_equality()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(1);
		sv.emplace_back(2);

		auto a = sv.begin();
		auto b = sv.begin();
		AGE_ASSERT(a == b);
		AGE_ASSERT((a != b) is_false);

		++b;
		AGE_ASSERT(a != b);

		++a;
		AGE_ASSERT(a == b);

		++a;
		AGE_ASSERT(a == sv.end());
	}

	// ============================================================
	// idx uniqueness invariant under emplace/remove cycles
	// ============================================================
	void
	test_idx_uniqueness_invariant()
	{
		age::sparse_vector<uint32> sv;
		auto					   live_set = age::unordered_map<std::size_t, uint32>{};

		uint32 seed		 = 0xCAFEBABE;
		auto   next_rand = [&]() -> uint32 {
			seed = seed * 1103515245u + 12345u;
			return seed;
		};

		for (uint32 step = 0; step < 50000; ++step)
		{
			c_auto op = next_rand() % 10;

			if (op < 7 || live_set.empty())
			{
				// emplace
				c_auto value = step;
				c_auto idx	 = sv.emplace_back(value);

				AGE_ASSERT(live_set.contains(idx) is_false);

				AGE_ASSERT(sv.is_live(idx));
				AGE_ASSERT(sv[idx] == value);

				live_set.emplace(idx, value);
			}
			else
			{
				c_auto pick = next_rand() % live_set.size();
				auto   it	= live_set.begin();
				for (uint32 i = 0; i < pick; ++i)
				{
					++it;
				}
				c_auto victim_idx = it->first;

				sv.remove(victim_idx);
				live_set.erase(victim_idx);

				AGE_ASSERT(sv.is_live(victim_idx) is_false);
			}

			if (step % 100 == 0)
			{
				AGE_ASSERT(sv.size() == live_set.size());

				uint32 iter_count = 0;
				for (auto it = sv.begin(); it != sv.end(); ++it)
				{
					c_auto idx = it.idx();
					AGE_ASSERT(live_set.contains(idx));
					AGE_ASSERT(live_set[idx] == *it);
					++iter_count;
				}
				AGE_ASSERT(iter_count == live_set.size());
			}
		}
	}

	// ============================================================
	// no-overlap invariant: every idx appears at most once
	// ============================================================
	void
	test_emplace_returns_unique_idx()
	{
		age::sparse_vector<uint32> sv;
		auto					   seen = age::vector<bool>{};

		for (uint32 round = 0; round < 1000; ++round)
		{
			auto current_idxs = age::vector<std::size_t>{};
			for (uint32 i = 0; i < 500; ++i)
			{
				c_auto idx = sv.emplace_back(i);

				if (idx >= seen.size()) { seen.resize(idx + 1, false); }
				AGE_ASSERT(seen[idx] is_false);
				seen[idx] = true;
				current_idxs.emplace_back(idx);
			}

			for (auto idx : current_idxs)
			{
				sv.remove(idx);
				seen[idx] = false;
			}

			AGE_ASSERT(sv.size() == 0);
		}
	}

	void
	test_emplace_to_full_capacity()
	{
		auto sv	  = age::sparse_vector<uint32>::gen_reserve(64);
		auto seen = age::vector<bool>(64, false);

		// fill cap exactly (no grow expected)
		for (uint32 i = 0; i < 64; ++i)
		{
			c_auto idx = sv.emplace_back(i);
			AGE_ASSERT(idx < 64);
			AGE_ASSERT(seen[idx] is_false);
			seen[idx] = true;
		}

		AGE_ASSERT(sv.size() == 64);
		AGE_ASSERT(sv.capacity() == 64);	// no grow happened
		AGE_ASSERT(sv.hole_count() == 0);

		// every idx hit exactly once
		for (uint32 i = 0; i < 64; ++i)
		{
			AGE_ASSERT(seen[i]);
		}

		// next emplace triggers grow
		c_auto next_idx = sv.emplace_back(999);
		AGE_ASSERT(next_idx == 64);
		AGE_ASSERT(sv.capacity() > 64);
	}

	void
	test_repeated_fill_and_reserve()
	{
		age::sparse_vector<uint32> sv;
		auto					   seen = age::vector<bool>{};

		// round 1: empty -> 100 emplace -> 50 remove -> reserve(500) -> fill to end
		auto idxs = age::vector<std::size_t>{};
		for (uint32 i = 0; i < 100; ++i)
		{
			idxs.emplace_back(sv.emplace_back(i));
		}

		for (uint32 i = 0; i < 50; ++i)
		{
			sv.remove(idxs[i]);
		}
		AGE_ASSERT(sv.size() == 50);

		sv.reserve(500);

		// fill to end while verifying idx uniqueness
		seen.resize(sv.capacity(), false);
		for (auto it = sv.begin(); it != sv.end(); ++it)
		{
			seen[it.idx()] = true;
		}

		c_auto holes = sv.hole_count();
		for (std::size_t i = 0; i < holes; ++i)
		{
			c_auto idx = sv.emplace_back(static_cast<uint32>(i));
			AGE_ASSERT(idx < sv.capacity());
			AGE_ASSERT(seen[idx] is_false);
			seen[idx] = true;
		}

		AGE_ASSERT(sv.hole_count() == 0);
		AGE_ASSERT(sv.size() == sv.capacity());
		for (std::size_t i = 0; i < sv.capacity(); ++i)
		{
			AGE_ASSERT(seen[i]);
		}
	}

	void
	test_grow_at_boundary()
	{
		// count == cap - 1 -> use last hole -> reach cap -> next emplace grows
		auto sv = age::sparse_vector<uint32>::gen_reserve(8);

		for (uint32 i = 0; i < 7; ++i)
		{
			sv.emplace_back(i);
		}
		AGE_ASSERT(sv.size() == 7);
		AGE_ASSERT(sv.capacity() == 8);
		AGE_ASSERT(sv.hole_count() == 1);

		// fill last hole (no grow)
		c_auto last = sv.emplace_back(7);
		AGE_ASSERT(last == 7);
		AGE_ASSERT(sv.size() == 8);
		AGE_ASSERT(sv.capacity() == 8);
		AGE_ASSERT(sv.hole_count() == 0);

		// next emplace = grow
		c_auto first_after_grow = sv.emplace_back(100);
		AGE_ASSERT(first_after_grow == 8);
		AGE_ASSERT(sv.capacity() > 8);

		// existing data preserved
		for (uint32 i = 0; i < 8; ++i)
		{
			AGE_ASSERT(sv[i] == i);
		}
	}

	void
	test_multiple_grow_cycles()
	{
		age::sparse_vector<uint32> sv;
		auto					   seen = age::vector<bool>{};

		// each cycle: fill to end -> remove half -> fill to end again (forces grow)
		for (uint32 cycle = 0; cycle < 10; ++cycle)
		{
			c_auto target_size = (cycle + 1) * 200;

			while (sv.size() < target_size)
			{
				c_auto idx = sv.emplace_back(cycle);
				if (idx >= seen.size()) { seen.resize(idx + 1, false); }
				AGE_ASSERT(seen[idx] is_false);
				seen[idx] = true;
			}

			// at hole_count == 0, size must equal cap exactly
			if (sv.hole_count() == 0)
			{
				AGE_ASSERT(sv.size() == sv.capacity());
			}

			// every idx hit at most once
			std::size_t live_count = 0;
			for (auto it = sv.begin(); it != sv.end(); ++it)
			{
				AGE_ASSERT(seen[it.idx()]);
				++live_count;
			}
			AGE_ASSERT(live_count == sv.size());
		}
	}

	struct complex_value
	{
		age::vector<uint32>				  ids;
		age::unordered_map<int32, uint64> map;
		uint32							  version = 0;

		complex_value() = default;

		complex_value(uint32 v) : version{ v }
		{
			for (uint32 i = 0; i < v; ++i)
			{
				ids.emplace_back(i);
			}
			for (uint32 i = 0; i < v; ++i)
			{
				map.emplace(static_cast<int32>(i), uint64{ i } * 100);
			}
		}

		bool
		operator==(const complex_value& other) const
		{
			if (version != other.version) { return false; }
			if (ids.size() != other.ids.size()) { return false; }
			for (std::size_t i = 0; i < ids.size(); ++i)
			{
				if (ids[i] != other.ids[i]) { return false; }
			}
			return true;
		}
	};

	struct alignas(32) over_aligned
	{
		uint8  data[32];
		uint32 marker = 0;

		over_aligned() = default;

		over_aligned(uint32 m) : marker{ m }
		{
			for (uint32 i = 0; i < 32; ++i)
			{
				data[i] = static_cast<uint8>(m + i);
			}
		}
	};

	struct virtual_value
	{
		uint32 base		= 0;
		virtual_value() = default;

		virtual_value(uint32 v) : base{ v } { }

		virtual ~virtual_value() = default;

		virtual uint32
		compute() const
		{ return base; }
	};

	struct derived_value : virtual_value
	{
		uint32 extra	= 0;
		derived_value() = default;

		derived_value(uint32 b, uint32 e) : virtual_value{ b }, extra{ e } { }

		uint32
		compute() const override
		{ return base + extra; }
	};

	// ============================================================
	void
	test_complex_value_lifecycle()
	{
		age::sparse_vector<complex_value> sv;

		for (uint32 i = 1; i <= 20; ++i)
		{
			sv.emplace_back(i);
		}

		for (uint32 i = 0; i < 20; ++i)
		{
			AGE_ASSERT(sv[i].version == i + 1);
			AGE_ASSERT(sv[i].ids.size() == i + 1);
		}

		auto copy = sv;
		for (uint32 i = 0; i < 20; ++i)
		{
			AGE_ASSERT(copy[i] == sv[i]);
		}

		sv.remove(5);
		sv.remove(10);

		sv.emplace_back(complex_value{ 99 });
		sv.emplace_back(complex_value{ 100 });

		for (uint32 i = 0; i < 1000; ++i)
		{
			sv.emplace_back(i + 1000);
		}

		AGE_ASSERT(sv[0].version == 1);
		AGE_ASSERT(sv[19].version == 20);
	}

	// ============================================================
	void
	test_over_aligned_value()
	{
		age::sparse_vector<over_aligned> sv;

		for (uint32 i = 0; i < 100; ++i)
		{
			sv.emplace_back(i);
		}

		for (uint32 i = 0; i < 100; ++i)
		{
			AGE_ASSERT(reinterpret_cast<std::uintptr_t>(&sv[i]) % 32 == 0);
			AGE_ASSERT(sv[i].marker == i);
			for (uint32 j = 0; j < 32; ++j)
			{
				AGE_ASSERT(sv[i].data[j] == static_cast<uint8>(i + j));
			}
		}
	}

	// ============================================================
	void
	test_polymorphic_holder()
	{
		age::sparse_vector<std::unique_ptr<virtual_value>> sv;

		sv.emplace_back(std::make_unique<virtual_value>(10));
		sv.emplace_back(std::make_unique<derived_value>(20, 5));

		AGE_ASSERT(sv[0]->compute() == 10);
		AGE_ASSERT(sv[1]->compute() == 25);

		sv.remove(0);
		AGE_ASSERT(sv.size() == 1);

		for (uint32 i = 0; i < 100; ++i)
		{
			sv.emplace_back(std::make_unique<virtual_value>(i));
		}

		AGE_ASSERT(sv[1]->compute() == 25);
	}

	// ============================================================
	// single slot churning - LIFO reuse, no cap growth
	// ============================================================
	void
	test_single_slot_churn()
	{
		age::sparse_vector<uint32> sv;
		c_auto					   idx = sv.emplace_back(0);

		c_auto cap_before = sv.capacity();

		for (uint32 i = 0; i < 100000; ++i)
		{
			sv.remove(idx);
			c_auto reused = sv.emplace_back(i);
			AGE_ASSERT(reused == idx);
		}

		AGE_ASSERT(sv.size() == 1);
		AGE_ASSERT(sv.capacity() == cap_before);
	}

	// ============================================================
	// emplace argument forwarding (lvalue / rvalue / pure rvalue)
	// ============================================================
	void
	test_emplace_forwarding()
	{
		age::sparse_vector<age::vector<uint32>> sv;

		auto src = age::vector<uint32>{};
		src.emplace_back(1);
		src.emplace_back(2);

		sv.emplace_back(src);
		AGE_ASSERT(src.size() == 2);
		AGE_ASSERT(sv[0].size() == 2);

		sv.emplace_back(std::move(src));
		AGE_ASSERT(src.size() == 0);
		AGE_ASSERT(sv[1].size() == 2);

		sv.emplace_back(age::vector<uint32>{});
		AGE_ASSERT(sv.size() == 3);
		AGE_ASSERT(sv[2].size() == 0);
	}

	// ============================================================
	// emplace with no args (default construction)
	// ============================================================
	void
	test_emplace_default()
	{
		age::sparse_vector<age::vector<uint32>> sv;
		sv.emplace_back();

		AGE_ASSERT(sv.size() == 1);
		AGE_ASSERT(sv[0].size() == 0);
	}

	// ============================================================
	// write isolation - no neighbor slot corruption
	// ============================================================
	void
	test_write_isolation()
	{
		// uint8 with size_type(8) hole storage - alignment/padding boundary
		age::sparse_vector<uint8> sv;
		for (uint32 i = 0; i < 100; ++i)
		{
			sv.emplace_back(static_cast<uint8>(i));
		}

		sv[50] = 0xFF;

		for (uint32 i = 0; i < 100; ++i)
		{
			if (i == 50) { AGE_ASSERT(sv[i] == 0xFF); }
			else
			{
				AGE_ASSERT(sv[i] == static_cast<uint8>(i));
			}
		}
	}

	// ============================================================
	// clear preserves cap, subsequent fill grows correctly
	// ============================================================
	void
	test_clear_then_grow()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 100; ++i)
		{
			sv.emplace_back(i);
		}

		c_auto cap_before = sv.capacity();
		sv.clear();
		AGE_ASSERT(sv.capacity() == cap_before);

		c_auto target = cap_before * 3;
		for (uint32 i = 0; i < target; ++i)
		{
			sv.emplace_back(i + 1000);
		}

		AGE_ASSERT(sv.capacity() > cap_before);
		AGE_ASSERT(sv.size() == target);
		for (uint32 i = 0; i < target; ++i)
		{
			AGE_ASSERT(sv[i] == i + 1000);
		}
	}

	// ============================================================
	// reserve(0) and reserve(<= current cap) are no-ops
	// ============================================================
	void
	test_reserve_noop()
	{
		age::sparse_vector<uint32> sv;
		sv.emplace_back(1);

		c_auto cap_before = sv.capacity();
		auto*  ptr_before = &sv[0];

		sv.reserve(0);
		AGE_ASSERT(sv.capacity() == cap_before);
		AGE_ASSERT(&sv[0] == ptr_before);

		sv.reserve(cap_before);
		AGE_ASSERT(sv.capacity() == cap_before);
		AGE_ASSERT(&sv[0] == ptr_before);
	}

	// ============================================================
	// gen_reserve(0) equivalent to default
	// ============================================================
	void
	test_gen_reserve_zero()
	{
		auto sv = age::sparse_vector<uint32>::gen_reserve(0);

		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.capacity() == 0);
		AGE_ASSERT(sv.empty());
		AGE_ASSERT(sv.begin() == sv.end());

		c_auto idx = sv.emplace_back(42);
		AGE_ASSERT(sv.size() == 1);
		AGE_ASSERT(sv[idx] == 42);
		AGE_ASSERT(sv.capacity() > 0);
	}

	// ============================================================
	// moved-from container can be reused
	// ============================================================
	void
	test_use_after_move()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 10; ++i)
		{
			sv.emplace_back(i);
		}

		auto moved = std::move(sv);
		AGE_ASSERT(sv.size() == 0);
		AGE_ASSERT(sv.capacity() == 0);

		c_auto idx = sv.emplace_back(99);
		AGE_ASSERT(sv.size() == 1);
		AGE_ASSERT(sv[idx] == 99);

		// moved retains data
		AGE_ASSERT(moved.size() == 10);
		for (uint32 i = 0; i < 10; ++i)
		{
			AGE_ASSERT(moved[i] == i);
		}
	}

	// ============================================================
	// const ref iteration
	// ============================================================
	void
	test_const_iteration()
	{
		age::sparse_vector<uint32> sv;
		for (uint32 i = 0; i < 5; ++i)
		{
			sv.emplace_back(i);
		}

		c_auto sum_const = [](const age::sparse_vector<uint32>& v) -> uint32 {
			uint32 s = 0;
			for (auto x : v)
			{
				s += x;
			}
			return s;
		};

		AGE_ASSERT(sum_const(sv) == 0 + 1 + 2 + 3 + 4);
	}

	// ============================================================
	void
	run_test()
	{
		test_default_constructor();
		test_gen_reserve();
		test_emplace_back_single();
		test_emplace_back_sequential();
		test_emplace_back_grow();
		test_remove_single();
		test_remove_reuses_hole();
		test_remove_lifo_order();
		test_operator_bracket();
		test_operator_bracket_handle();
		test_remove_handle();
		test_front_back();
		test_front_back_with_holes();
		test_clear();
		test_reserve();
		test_reserve_preserves_data();
		test_reserve_preserves_holes();
		test_is_live();
		test_hole_count();
		test_iterator_basic();
		test_iterator_skips_holes();
		test_iterator_empty();
		test_const_iterator();
		test_iterator_const_conversion();
		test_iterator_idx();
		test_reverse_iterator_basic();
		test_reverse_iterator_skips_holes();
		test_reverse_iterator_empty();
		test_const_reverse_iterator();
		test_iterator_post_increment();
		test_copy_constructor();
		test_copy_independence();
		test_move_constructor();
		test_copy_assignment();
		test_move_assignment();
		test_self_copy_assignment();
		test_self_move_assignment();
		test_non_trivial_destruction();
		test_non_trivial_clear();
		test_non_trivial_copy();
		test_non_trivial_grow();
		test_non_trivial_relocate_with_holes();
		test_movable_only();
		test_boundary_cap_64();
		test_boundary_cap_65();
		test_boundary_caps();
		test_word_boundary_front_back();
		test_emplace_after_full_removal();
		test_interleaved_emplace_remove();
		test_stress_sequential();
		test_stress_emplace_remove_cycles();
		test_stress_random_ops();
		test_stress_copy_under_load();
		test_stress_non_trivial_lifecycle();
		test_stress_grow_chain();

		test_iterator_concept();
		test_range_concept();
		test_ranges_find();
		test_ranges_predicates();
		test_ranges_for_each();
		test_views_filter();
		test_views_transform();
		test_iterator_mutation();
		test_iterator_copy_to_vector();
		test_iterator_equality();

		test_idx_uniqueness_invariant();
		test_emplace_returns_unique_idx();

		test_repeated_fill_and_reserve();
		test_grow_at_boundary();
		test_multiple_grow_cycles();

		test_complex_value_lifecycle();
		test_over_aligned_value();
		test_polymorphic_holder();


		test_single_slot_churn();
		test_emplace_forwarding();
		test_emplace_default();
		test_write_isolation();
		test_clear_then_grow();
		test_reserve_noop();
		test_gen_reserve_zero();
		test_use_after_move();
		test_const_iteration();
	}
}	 // namespace age_test::data_structure::sparse_vector