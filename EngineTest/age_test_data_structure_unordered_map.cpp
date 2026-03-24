#include "age_test_pch.hpp"
#include "age_test.hpp"

namespace age_test::data_structure::unordered_map
{
	// ============================================================
	// non-trivial value type for testing destroy/construct paths
	// ============================================================
	struct tracked_value
	{
		int				  value		  = 0;
		inline static int alive_count = 0;

		tracked_value() noexcept : value(0) { ++alive_count; }

		tracked_value(int v) noexcept : value(v) { ++alive_count; }

		tracked_value(const tracked_value& o) noexcept : value(o.value) { ++alive_count; }

		tracked_value(tracked_value&& o) noexcept : value(std::exchange(o.value, -1)) { ++alive_count; }

		~tracked_value() noexcept { --alive_count; }

		tracked_value&
		operator=(const tracked_value& o) noexcept
		{
			value = o.value;
			return *this;
		}

		tracked_value&
		operator=(tracked_value&& o) noexcept
		{
			value = std::exchange(o.value, -1);
			return *this;
		}

		bool
		operator==(const tracked_value& o) const noexcept
		{ return value == o.value; }
	};

	// ============================================================
	// 1. default constructor
	// ============================================================
	void
	test_default_constructor()
	{
		age::unordered_map<uint64, int> map;

		AGE_ASSERT(map.size() == 0);
		AGE_ASSERT(map.empty());
		AGE_ASSERT(map.is_empty());
		AGE_ASSERT(map.begin() == map.end());
	}

	// ============================================================
	// 2. emplace
	// ============================================================
	void
	test_emplace()
	{
		age::unordered_map<uint64, int> map;

		// insert new
		auto [it1, inserted1] = map.emplace(1, 100);
		AGE_ASSERT(inserted1);
		AGE_ASSERT(it1->first == 1);
		AGE_ASSERT(it1->second == 100);
		AGE_ASSERT(map.size() == 1);

		// insert duplicate - should not overwrite
		auto [it2, inserted2] = map.emplace(1, 999);
		AGE_ASSERT(inserted2 is_false);
		AGE_ASSERT(it2->second == 100);
		AGE_ASSERT(map.size() == 1);

		// insert multiple
		map.emplace(2, 200);
		map.emplace(3, 300);
		AGE_ASSERT(map.size() == 3);
	}

	// ============================================================
	// 3. insert (const& and &&)
	// ============================================================
	void
	test_insert()
	{
		age::unordered_map<uint64, int> map;

		// const ref
		std::pair<const uint64, int> kv1{ 1, 100 };
		auto [it1, ok1] = map.insert(kv1);
		AGE_ASSERT(ok1);
		AGE_ASSERT(it1->second == 100);

		// rvalue
		auto [it2, ok2] = map.insert(std::pair<const uint64, int>{ 2, 200 });
		AGE_ASSERT(ok2);
		AGE_ASSERT(it2->second == 200);

		// duplicate
		auto [it3, ok3] = map.insert(kv1);
		AGE_ASSERT(ok3 is_false);

		AGE_ASSERT(map.size() == 2);
	}

	// ============================================================
	// 4. try_emplace
	// ============================================================
	void
	test_try_emplace()
	{
		age::unordered_map<uint64, int> map;

		// new key
		auto [it1, ok1] = map.try_emplace(1, 100);
		AGE_ASSERT(ok1);
		AGE_ASSERT(it1->second == 100);

		// existing key - value args should not be evaluated
		auto [it2, ok2] = map.try_emplace(1, 999);
		AGE_ASSERT(ok2 is_false);
		AGE_ASSERT(it2->second == 100);

		// default value
		auto [it3, ok3] = map.try_emplace(2);
		AGE_ASSERT(ok3);
		AGE_ASSERT(it3->second == 0);

		AGE_ASSERT(map.size() == 2);
	}

	// ============================================================
	// 5. operator[]
	// ============================================================
	void
	test_operator_bracket()
	{
		age::unordered_map<uint64, int> map;

		// creates default value
		map[1] = 100;
		AGE_ASSERT(map.size() == 1);
		AGE_ASSERT(map[1] == 100);

		// overwrite existing
		map[1] = 200;
		AGE_ASSERT(map[1] == 200);
		AGE_ASSERT(map.size() == 1);

		// read creates entry
		int val = map[42];
		AGE_ASSERT(val == 0);
		AGE_ASSERT(map.size() == 2);
	}

	// ============================================================
	// 6. find
	// ============================================================
	void
	test_find()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		// existing
		auto it = map.find(2);
		AGE_ASSERT(it != map.end());
		AGE_ASSERT(it->first == 2);
		AGE_ASSERT(it->second == 200);

		// non-existing
		auto it2 = map.find(999);
		AGE_ASSERT(it2 == map.end());

		// const find
		const auto& cmap = map;
		auto		cit	 = cmap.find(1);
		AGE_ASSERT(cit != cmap.end());
		AGE_ASSERT(cit->second == 100);

		auto cit2 = cmap.find(999);
		AGE_ASSERT(cit2 == cmap.end());
	}

	// ============================================================
	// 7. at
	// ============================================================
	void
	test_at()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(5, 500);

		AGE_ASSERT(map.at(5) == 500);
	}

	// ============================================================
	// 8. contains
	// ============================================================
	void
	test_contains()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);

		AGE_ASSERT(map.contains(1));
		AGE_ASSERT(map.contains(2) is_false);
	}

	// ============================================================
	// 9. erase
	// ============================================================
	void
	test_erase()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		// erase existing
		AGE_ASSERT(map.erase(2) == 1);
		AGE_ASSERT(map.size() == 2);
		AGE_ASSERT(map.contains(2) is_false);

		// erase non-existing
		AGE_ASSERT(map.erase(999) == 0);
		AGE_ASSERT(map.size() == 2);

		// erase all
		AGE_ASSERT(map.erase(1) == 1);
		AGE_ASSERT(map.erase(3) == 1);
		AGE_ASSERT(map.empty());

		// insert after erase - reuses slot
		map.emplace(10, 1000);
		AGE_ASSERT(map.size() == 1);
		AGE_ASSERT(map.find(10)->second == 1000);
	}

	// ============================================================
	// 10. clear
	// ============================================================
	void
	test_clear()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 20; ++i)
			map.emplace(i, static_cast<int>(i * 10));

		AGE_ASSERT(map.size() == 20);

		map.clear();

		AGE_ASSERT(map.size() == 0);
		AGE_ASSERT(map.empty());
		AGE_ASSERT(map.begin() == map.end());
		AGE_ASSERT(map.find(0) == map.end());

		// insert after clear
		map.emplace(42, 420);
		AGE_ASSERT(map.size() == 1);
		AGE_ASSERT(map[42] == 420);
	}

	// ============================================================
	// 11. size / empty / is_empty
	// ============================================================
	void
	test_size_empty()
	{
		age::unordered_map<uint64, int> map;

		AGE_ASSERT(map.size() == 0);
		AGE_ASSERT(map.empty());
		AGE_ASSERT(map.is_empty());

		map.emplace(1, 1);

		AGE_ASSERT(map.size() == 1);
		AGE_ASSERT(map.empty() is_false);
		AGE_ASSERT(map.is_empty() is_false);

		// size with custom return type
		AGE_ASSERT(map.size<uint32>() == 1u);
	}

	// ============================================================
	// 12. iterator - begin, end, cbegin, cend
	// ============================================================
	void
	test_iterator()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		// range-for
		int sum		= 0;
		int key_sum = 0;
		for (auto& [k, v] : map)
		{
			key_sum += static_cast<int>(k);
			sum		+= v;
		}
		AGE_ASSERT(key_sum == 6);
		AGE_ASSERT(sum == 600);

		// manual iteration
		uint32 count = 0;
		for (auto it = map.begin(); it != map.end(); ++it)
			++count;
		AGE_ASSERT(count == 3);

		// const iteration
		const auto& cmap = map;
		int			csum = 0;
		for (auto it = cmap.begin(); it != cmap.end(); ++it)
			csum += it->second;
		AGE_ASSERT(csum == 600);

		// cbegin / cend
		int csum2 = 0;
		for (auto it = map.cbegin(); it != map.cend(); ++it)
			csum2 += it->second;
		AGE_ASSERT(csum2 == 600);
	}

	// ============================================================
	// 13. iterator dereference - operator* and operator->
	// ============================================================
	void
	test_iterator_deref()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(42, 420);

		auto it = map.find(42);

		// operator*
		auto& ref = *it;
		AGE_ASSERT(ref.first == 42);
		AGE_ASSERT(ref.second == 420);

		// operator->
		AGE_ASSERT(it->first == 42);
		AGE_ASSERT(it->second == 420);

		// modify via iterator
		it->second = 999;
		AGE_ASSERT(map[42] == 999);
	}

	// ============================================================
	// 14. copy constructor
	// ============================================================
	void
	test_copy_constructor()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 50; ++i)
			map.emplace(i, static_cast<int>(i * 10));

		auto copy = map;

		AGE_ASSERT(copy.size() == 50);
		for (uint64 i = 0; i < 50; ++i)
			AGE_ASSERT(copy.find(i)->second == static_cast<int>(i * 10));

		// independent - modify original, copy unchanged
		map.erase(0);
		AGE_ASSERT(map.size() == 49);
		AGE_ASSERT(copy.size() == 50);
		AGE_ASSERT(copy.contains(0));
	}

	// ============================================================
	// 15. move constructor
	// ============================================================
	void
	test_move_constructor()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 50; ++i)
			map.emplace(i, static_cast<int>(i * 10));

		auto moved = std::move(map);

		AGE_ASSERT(moved.size() == 50);
		AGE_ASSERT(map.empty());

		for (uint64 i = 0; i < 50; ++i)
			AGE_ASSERT(moved.find(i)->second == static_cast<int>(i * 10));
	}

	// ============================================================
	// 16. copy assignment
	// ============================================================
	void
	test_copy_assignment()
	{
		age::unordered_map<uint64, int> a;
		a.emplace(1, 100);
		a.emplace(2, 200);

		age::unordered_map<uint64, int> b;
		b.emplace(99, 99);

		b = a;

		AGE_ASSERT(b.size() == 2);
		AGE_ASSERT(b[1] == 100);
		AGE_ASSERT(b[2] == 200);
		AGE_ASSERT(b.contains(99) is_false);

		// self assignment
		b = b;
		AGE_ASSERT(b.size() == 2);
	}

	// ============================================================
	// 17. move assignment
	// ============================================================
	void
	test_move_assignment()
	{
		age::unordered_map<uint64, int> a;
		a.emplace(1, 100);
		a.emplace(2, 200);

		age::unordered_map<uint64, int> b;
		b.emplace(99, 99);

		b = std::move(a);

		AGE_ASSERT(b.size() == 2);
		AGE_ASSERT(b[1] == 100);
		AGE_ASSERT(a.empty());
	}

	// ============================================================
	// 18. reserve
	// ============================================================
	void
	test_reserve()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);
		map.emplace(2, 200);

		map.reserve(1000);

		// data preserved after reserve
		AGE_ASSERT(map.size() == 2);
		AGE_ASSERT(map[1] == 100);
		AGE_ASSERT(map[2] == 200);

		// reserve smaller does nothing
		map.reserve(10);
		AGE_ASSERT(map.size() == 2);
	}

	// ============================================================
	// 19. resize - stress test filling blocks
	// ============================================================
	void
	test_resize_stress()
	{
		age::unordered_map<uint64, uint64> map;

		// insert enough to trigger multiple resizes
		constexpr uint64 N = 500;
		for (uint64 i = 0; i < N; ++i)
			map.emplace(i, i * 7);

		AGE_ASSERT(map.size() == N);

		// all values correct after resizes
		for (uint64 i = 0; i < N; ++i)
		{
			auto it = map.find(i);
			AGE_ASSERT(it != map.end());
			AGE_ASSERT(it->second == i * 7);
		}
	}

	// ============================================================
	// 20. non-trivial type - destroy tracking
	// ============================================================
	void
	test_non_trivial_type()
	{
		tracked_value::alive_count = 0;

		{
			age::unordered_map<uint64, tracked_value> map;
			map.emplace(1, tracked_value(100));
			map.emplace(2, tracked_value(200));
			map.emplace(3, tracked_value(300));

			AGE_ASSERT(map.size() == 3);
			// alive_count includes temporaries that may have been destroyed,
			// but at least 3 should be alive in the map
			auto count_after_insert = tracked_value::alive_count;
			AGE_ASSERT(count_after_insert >= 3);

			// erase one
			map.erase(2);
			AGE_ASSERT(tracked_value::alive_count == count_after_insert - 1);

			// clear
			map.clear();
			AGE_ASSERT(tracked_value::alive_count == count_after_insert - 3);
		}

		// destructor should clean up all remaining
		AGE_ASSERT(tracked_value::alive_count == 0);
	}

	// ============================================================
	// 21. non-trivial type - copy/move
	// ============================================================
	void
	test_non_trivial_copy_move()
	{
		tracked_value::alive_count = 0;

		{
			age::unordered_map<uint64, tracked_value> map;
			map.emplace(1, tracked_value(100));
			map.emplace(2, tracked_value(200));

			// copy
			auto copy = map;
			AGE_ASSERT(copy.size() == 2);
			AGE_ASSERT(copy.find(1)->second.value == 100);

			// move
			auto moved = std::move(copy);
			AGE_ASSERT(moved.size() == 2);
			AGE_ASSERT(copy.empty());
		}

		AGE_ASSERT(tracked_value::alive_count == 0);
	}

	// ============================================================
	// 22. hash collision - same block stress
	// ============================================================
	void
	test_hash_collision()
	{
		// keys that hash to the same block (multiples of 32)
		age::unordered_map<uint64, int> map;

		for (uint64 i = 0; i < 30; ++i)
		{
			map.emplace(i * 32, static_cast<int>(i));
		}

		AGE_ASSERT(map.size() == 30);

		for (uint64 i = 0; i < 30; ++i)
		{
			auto it = map.find(i * 32);
			AGE_ASSERT(it != map.end());
			AGE_ASSERT(it->second == static_cast<int>(i));
		}
	}

	// ============================================================
	// 23. erase + re-insert cycle
	// ============================================================
	void
	test_erase_reinsert()
	{
		age::unordered_map<uint64, int> map;

		// fill
		for (uint64 i = 0; i < 100; ++i)
			map.emplace(i, static_cast<int>(i));

		// erase half
		for (uint64 i = 0; i < 100; i += 2)
			map.erase(i);

		AGE_ASSERT(map.size() == 50);

		// re-insert erased keys with new values
		for (uint64 i = 0; i < 100; i += 2)
			map.emplace(i, static_cast<int>(i + 1000));

		AGE_ASSERT(map.size() == 100);

		// verify all
		for (uint64 i = 0; i < 100; ++i)
		{
			auto it = map.find(i);
			AGE_ASSERT(it != map.end());
			if (i % 2 == 0)
				AGE_ASSERT(it->second == static_cast<int>(i + 1000));
			else
				AGE_ASSERT(it->second == static_cast<int>(i));
		}
	}

	// ============================================================
	// 24. iterator after erase - skips erased slots
	// ============================================================
	void
	test_iterator_after_erase()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		map.erase(2);

		int	   sum	 = 0;
		uint32 count = 0;
		for (auto& [k, v] : map)
		{
			sum += v;
			++count;
		}

		AGE_ASSERT(count == 2);
		AGE_ASSERT(sum == 400);	   // 100 + 300
	}

	// ============================================================
	// 25. empty map operations
	// ============================================================
	void
	test_empty_map()
	{
		age::unordered_map<uint64, int> map;

		AGE_ASSERT(map.find(0) == map.end());
		AGE_ASSERT(map.contains(0) is_false);
		AGE_ASSERT(map.erase(0) == 0);
		AGE_ASSERT(map.begin() == map.end());

		// clear on empty is safe
		map.clear();
		AGE_ASSERT(map.empty());
	}

	// ============================================================
	// 26. initializer_list constructor
	// ============================================================
	void
	test_initializer_list()
	{
		age::unordered_map<uint64, int> map = {
			{ 1, 100 },
			{ 2, 200 },
			{ 3, 300 },
		};

		AGE_ASSERT(map.size() == 3);
		AGE_ASSERT(map[1] == 100);
		AGE_ASSERT(map[2] == 200);
		AGE_ASSERT(map[3] == 300);
	}

	// ============================================================
	// 27. iterator range constructor
	// ============================================================
	void
	test_range_constructor()
	{
		std::vector<std::pair<const uint64, int>> vec = {
			{ 1, 100 },
			{ 2, 200 },
			{ 3, 300 },
		};

		age::unordered_map<uint64, int> map(vec.begin(), vec.end());

		AGE_ASSERT(map.size() == 3);
		AGE_ASSERT(map[1] == 100);
		AGE_ASSERT(map[2] == 200);
		AGE_ASSERT(map[3] == 300);
	}

	// ============================================================
	// 28. block_count
	// ============================================================
	void
	test_block_count()
	{
		age::unordered_map<uint64, int> map;

		// default capacity = 32, block = 1
		AGE_ASSERT(map.block_count() == 1);

		// reserve to trigger more blocks
		map.reserve(100);
		AGE_ASSERT(map.block_count() >= 4);	   // 128 / 32 = 4
	}

	// ============================================================
	// 29. hash_function / key_eq / get_allocator
	// ============================================================
	void
	test_accessors()
	{
		age::unordered_map<uint64, int> map;

		auto h = map.hash_function();
		auto e = map.key_eq();
		auto a = map.get_allocator();

		// hash produces consistent output
		AGE_ASSERT(h(42) == h(42));
		AGE_ASSERT(h(1) != h(2));

		// key_eq works
		AGE_ASSERT(e(1, 1));
		AGE_ASSERT(e(1, 2) is_false);

		(void)a;	// just check it compiles
	}

	// ============================================================
	// 30. large scale - 10000 entries
	// ============================================================
	void
	test_large_scale()
	{
		age::unordered_map<uint64, uint64> map;

		constexpr uint64 N = 10000;

		for (uint64 i = 0; i < N; ++i)
			map.emplace(i, i * 13);

		AGE_ASSERT(map.size() == N);

		for (uint64 i = 0; i < N; ++i)
		{
			AGE_ASSERT(map.contains(i));
			AGE_ASSERT(map.find(i)->second == i * 13);
		}

		// erase all
		for (uint64 i = 0; i < N; ++i)
			AGE_ASSERT(map.erase(i) == 1);

		AGE_ASSERT(map.empty());
	}

	// ============================================================
	// 31. real collision - identity hash forces same block
	// ============================================================
	struct identity_hash
	{
		constexpr uint64
		operator()(uint64 key) const noexcept
		{
			return key;
		}
	};

	void
	test_real_collision()
	{
		age::unordered_map<uint64, int, identity_hash> map;

		for (uint64 i = 0; i < 32; ++i)
			map.emplace(i, static_cast<int>(i));

		AGE_ASSERT(map.size() == 32);

		map.emplace(32, 320);
		AGE_ASSERT(map.size() == 33);

		for (uint64 i = 0; i <= 32; ++i)
		{
			auto it = map.find(i);
			AGE_ASSERT(it != map.end());
			AGE_ASSERT(it->second == static_cast<int>(i == 32 ? 320 : i));
		}
	}

	// ============================================================
	// 32. collision - erase + reinsert in full block
	// ============================================================
	void
	test_collision_erase_reinsert()
	{
		age::unordered_map<uint64, int, identity_hash> map;

		// fill block 0
		for (uint64 i = 0; i < 32; ++i)
			map.emplace(i, static_cast<int>(i));

		// erase middle
		map.erase(15);
		map.erase(16);
		AGE_ASSERT(map.size() == 30);

		// reinsert - should reuse freed slots, no resize
		map.emplace(15, 1500);
		map.emplace(16, 1600);
		AGE_ASSERT(map.size() == 32);
		AGE_ASSERT(map.find(15)->second == 1500);
		AGE_ASSERT(map.find(16)->second == 1600);
	}

	// ============================================================
	// 33. map<uint64, map<uint64, int>> - nested map
	// ============================================================
	void
	test_nested_map()
	{
		age::unordered_map<uint64, age::unordered_map<uint64, int>> outer;

		// build nested
		for (uint64 i = 0; i < 5; ++i)
		{
			outer[i] = age::unordered_map<uint64, int>{};
			for (uint64 j = 0; j < 10; ++j)
			{
				outer[i][j] = static_cast<int>(i * 100 + j);
			}
		}

		AGE_ASSERT(outer.size() == 5);

		// verify
		for (uint64 i = 0; i < 5; ++i)
		{
			auto it = outer.find(i);
			AGE_ASSERT(it != outer.end());
			AGE_ASSERT(it->second.size() == 10);

			for (uint64 j = 0; j < 10; ++j)
			{
				AGE_ASSERT(it->second.find(j)->second == static_cast<int>(i * 100 + j));
			}
		}

		// erase inner entry
		outer[2].erase(5);
		AGE_ASSERT(outer[2].size() == 9);
		AGE_ASSERT(outer[2].contains(5) is_false);

		// erase outer entry - inner map destructor must run
		outer.erase(3);
		AGE_ASSERT(outer.size() == 4);
		AGE_ASSERT(outer.contains(3) is_false);

		// copy nested
		auto copy = outer;
		AGE_ASSERT(copy.size() == 4);
		AGE_ASSERT(copy[0].size() == 10);
		AGE_ASSERT(copy[2].size() == 9);
	}

	// ============================================================
	// 34. vector<map<uint64, int>> - vector of maps
	// ============================================================
	void
	test_vector_of_map()
	{
		age::vector<age::unordered_map<uint64, int>> vec;

		for (int i = 0; i < 10; ++i)
		{
			vec.emplace_back();
			for (uint64 j = 0; j < 20; ++j)
			{
				vec.back()[j] = i * 1000 + static_cast<int>(j);
			}
		}

		AGE_ASSERT(vec.size() == 10);

		for (int i = 0; i < 10; ++i)
		{
			AGE_ASSERT(vec[i].size() == 20);
			for (uint64 j = 0; j < 20; ++j)
			{
				AGE_ASSERT(vec[i][j] == i * 1000 + static_cast<int>(j));
			}
		}

		// vector resize - maps must survive move
		vec.reserve(100);

		for (int i = 0; i < 10; ++i)
		{
			AGE_ASSERT(vec[i].size() == 20);
			AGE_ASSERT(vec[i][0] == i * 1000);
		}

		// pop - map destructor runs
		vec.pop_back();
		AGE_ASSERT(vec.size() == 9);
	}

	// ============================================================
	// 35. map<uint64, vector<int>> - map of vectors
	// ============================================================
	void
	test_map_of_vector()
	{
		age::unordered_map<uint64, age::vector<int>> map;

		for (uint64 i = 0; i < 10; ++i)
		{
			map[i] = age::vector<int>{};
			for (int j = 0; j < 15; ++j)
			{
				map[i].emplace_back(static_cast<int>(i * 100 + j));
			}
		}

		AGE_ASSERT(map.size() == 10);

		for (uint64 i = 0; i < 10; ++i)
		{
			auto it = map.find(i);
			AGE_ASSERT(it != map.end());
			AGE_ASSERT(it->second.size() == 15);

			for (int j = 0; j < 15; ++j)
			{
				AGE_ASSERT(it->second[j] == static_cast<int>(i * 100 + j));
			}
		}

		// erase - vector destructor must run
		map.erase(5);
		AGE_ASSERT(map.size() == 9);
		AGE_ASSERT(map.contains(5) is_false);

		// clear - all vector destructors
		map.clear();
		AGE_ASSERT(map.empty());
	}

	// ============================================================
	// 36. map<uint64, vector<map<uint64, int>>> - deep nesting
	// ============================================================
	void
	test_deep_nesting()
	{
		age::unordered_map<uint64, age::vector<age::unordered_map<uint64, int>>> map;

		for (uint64 i = 0; i < 3; ++i)
		{
			map[i] = age::vector<age::unordered_map<uint64, int>>{};

			for (int j = 0; j < 4; ++j)
			{
				map[i].emplace_back();
				for (uint64 k = 0; k < 5; ++k)
				{
					map[i].back()[k] = static_cast<int>(i * 1000 + j * 100 + k);
				}
			}
		}

		AGE_ASSERT(map.size() == 3);
		AGE_ASSERT(map[0].size() == 4);
		AGE_ASSERT(map[0][0].size() == 5);
		AGE_ASSERT(map[1][2][3] == 1203);

		// copy deep
		auto copy = map;
		AGE_ASSERT(copy[1][2][3] == 1203);

		// modify original, copy unaffected
		map[1][2][3] = 9999;
		AGE_ASSERT(copy[1][2][3] == 1203);

		// clear - cascading destructors
		map.clear();
		AGE_ASSERT(map.empty());
		AGE_ASSERT(copy.size() == 3);
	}

	// ============================================================
	// 37. stress - rapid insert/erase cycle
	// ============================================================
	void
	test_stress_insert_erase_cycle()
	{
		age::unordered_map<uint64, uint64> map;

		// 10 rounds of fill + partial erase
		for (int round = 0; round < 10; ++round)
		{
			c_auto base = static_cast<uint64>(round * 1000);

			// insert 500
			for (uint64 i = 0; i < 500; ++i)
				map.emplace(base + i, i);

			// erase every 3rd
			for (uint64 i = 0; i < 500; i += 3)
				map.erase(base + i);
		}

		// verify surviving entries
		for (int round = 0; round < 10; ++round)
		{
			c_auto base = static_cast<uint64>(round * 1000);

			for (uint64 i = 0; i < 500; ++i)
			{
				if (i % 3 == 0)
				{
					AGE_ASSERT(map.contains(base + i) is_false);
				}
				else
				{
					AGE_ASSERT(map.find(base + i)->second == i);
				}
			}
		}

		// expected: 10 rounds * (500 - 167) = 3330
		AGE_ASSERT(map.size() == 3330);
	}

	// ============================================================
	// 38. stress - sequential key pattern (worst case for block fill)
	// ============================================================
	void
	test_stress_sequential()
	{
		age::unordered_map<uint64, int> map;

		constexpr uint64 N = 50000;

		for (uint64 i = 0; i < N; ++i)
			map[i] = static_cast<int>(i);

		AGE_ASSERT(map.size() == N);

		// spot check
		AGE_ASSERT(map[0] == 0);
		AGE_ASSERT(map[N / 2] == static_cast<int>(N / 2));
		AGE_ASSERT(map[N - 1] == static_cast<int>(N - 1));

		// full verify via iterator
		uint64 count = 0;
		for (auto& [k, v] : map)
		{
			AGE_ASSERT(v == static_cast<int>(k));
			++count;
		}
		AGE_ASSERT(count == N);
	}

	// ============================================================
	// 39. stress - identity hash, force maximum resizes
	// ============================================================
	void
	test_stress_collision_resize()
	{
		age::unordered_map<uint64, int, identity_hash> map;

		// all keys hash to low range, forcing repeated block fills + resizes
		for (uint64 i = 0; i < 200; ++i)
			map.emplace(i, static_cast<int>(i * 3));

		AGE_ASSERT(map.size() == 200);

		for (uint64 i = 0; i < 200; ++i)
		{
			auto it = map.find(i);
			AGE_ASSERT(it != map.end());
			AGE_ASSERT(it->second == static_cast<int>(i * 3));
		}
	}

	// ============================================================
	// 40. stress - interleaved insert/find/erase
	// ============================================================
	void
	test_stress_interleaved()
	{
		age::unordered_map<uint64, uint64> map;

		constexpr uint64 N = 10000;

		// phase 1: insert all
		for (uint64 i = 0; i < N; ++i)
			map.emplace(i, i * 7);

		// phase 2: erase odd, verify even
		for (uint64 i = 0; i < N; ++i)
		{
			if (i & 1)
				map.erase(i);
			else
				AGE_ASSERT(map.find(i)->second == i * 7);
		}

		AGE_ASSERT(map.size() == N / 2);

		// phase 3: re-insert odd with new values
		for (uint64 i = 1; i < N; i += 2)
			map.emplace(i, i * 11);

		AGE_ASSERT(map.size() == N);

		// phase 4: verify all
		for (uint64 i = 0; i < N; ++i)
		{
			auto it = map.find(i);
			AGE_ASSERT(it != map.end());
			if (i & 1)
				AGE_ASSERT(it->second == i * 11);
			else
				AGE_ASSERT(it->second == i * 7);
		}

		// phase 5: erase all via clear, then refill
		map.clear();
		AGE_ASSERT(map.empty());

		for (uint64 i = 0; i < N; ++i)
			map.emplace(i, i);

		AGE_ASSERT(map.size() == N);
	}

	// ============================================================
	// 41. stress - non-trivial type mass lifecycle
	// ============================================================
	void
	test_stress_non_trivial_lifecycle()
	{
		tracked_value::alive_count = 0;

		{
			age::unordered_map<uint64, tracked_value> map;

			// insert 1000
			for (uint64 i = 0; i < 1000; ++i)
				map.emplace(i, tracked_value(static_cast<int>(i)));

			AGE_ASSERT(map.size() == 1000);

			// erase 500
			for (uint64 i = 0; i < 1000; i += 2)
				map.erase(i);

			AGE_ASSERT(map.size() == 500);

			// copy
			auto copy = map;
			AGE_ASSERT(copy.size() == 500);

			// clear original
			map.clear();
			AGE_ASSERT(map.empty());

			// copy still alive
			for (uint64 i = 1; i < 1000; i += 2)
				AGE_ASSERT(copy.find(i)->second.value == static_cast<int>(i));

		}	 // both destructors fire

		AGE_ASSERT(tracked_value::alive_count == 0);
	}

	// ============================================================
	// 42. stress - copy/move under load
	// ============================================================
	void
	test_stress_copy_move()
	{
		age::unordered_map<uint64, uint64> map;

		for (uint64 i = 0; i < 5000; ++i)
			map.emplace(i, i * 13);

		// copy
		auto a = map;
		AGE_ASSERT(a.size() == 5000);

		// move
		auto b = std::move(a);
		AGE_ASSERT(b.size() == 5000);
		AGE_ASSERT(a.empty());

		// copy assign
		a = b;
		AGE_ASSERT(a.size() == 5000);

		// move assign
		age::unordered_map<uint64, uint64> c;
		c.emplace(999, 999);
		c = std::move(b);
		AGE_ASSERT(c.size() == 5000);
		AGE_ASSERT(b.empty());

		// verify integrity after all transfers
		for (uint64 i = 0; i < 5000; ++i)
		{
			AGE_ASSERT(a.find(i)->second == i * 13);
			AGE_ASSERT(c.find(i)->second == i * 13);
		}
	}

	// ============================================================
	// 43. ranges - views::filter
	// ============================================================
	void
	test_ranges_filter()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 20; ++i)
			map.emplace(i, static_cast<int>(i));

		int sum = 0;
		for (auto& [k, v] : map | std::views::filter([](auto& kv) { return kv.second % 2 == 0; }))
		{
			sum += v;
		}

		// 0 + 2 + 4 + 6 + 8 + 10 + 12 + 14 + 16 + 18 = 90
		AGE_ASSERT(sum == 90);
	}

	// ============================================================
	// 44. ranges - views::transform
	// ============================================================
	void
	test_ranges_transform()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 10);
		map.emplace(2, 20);
		map.emplace(3, 30);

		// extract values only
		int sum = 0;
		for (auto val : map | std::views::transform([](auto& kv) { return kv.second; }))
		{
			sum += val;
		}

		AGE_ASSERT(sum == 60);
	}

	// ============================================================
	// 45. ranges - views::keys / views::values
	// ============================================================
	void
	test_ranges_keys_values()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(10, 100);
		map.emplace(20, 200);
		map.emplace(30, 300);

		uint64 key_sum = 0;
		for (auto k : map | std::views::keys)
			key_sum += k;

		AGE_ASSERT(key_sum == 60);

		int val_sum = 0;
		for (auto v : map | std::views::values)
			val_sum += v;

		AGE_ASSERT(val_sum == 600);
	}

	// ============================================================
	// 46. ranges - views::filter + views::transform chained
	// ============================================================
	void
	test_ranges_chained()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 10; ++i)
			map.emplace(i, static_cast<int>(i * 10));

		// filter even keys, transform to doubled values
		int sum = 0;
		for (auto val : map
							| std::views::filter([](auto& kv) { return kv.first % 2 == 0; })
							| std::views::transform([](auto& kv) { return kv.second * 2; }))
		{
			sum += val;
		}

		// keys 0,2,4,6,8 -> values 0,20,40,60,80 -> doubled 0,40,80,120,160 -> sum 400
		AGE_ASSERT(sum == 400);
	}

	// ============================================================
	// 47. ranges - std::ranges::count_if
	// ============================================================
	void
	test_ranges_count_if()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 100; ++i)
			map.emplace(i, static_cast<int>(i));

		auto cnt = std::ranges::count_if(map, [](auto& kv) { return kv.second >= 50; });

		AGE_ASSERT(cnt == 50);
	}

	// ============================================================
	// 48. ranges - std::ranges::find_if
	// ============================================================
	void
	test_ranges_find_if()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		auto it = std::ranges::find_if(map, [](auto& kv) { return kv.second == 200; });

		AGE_ASSERT(it != map.end());
		AGE_ASSERT(it->first == 2);
	}

	// ============================================================
	// 49. ranges - std::ranges::for_each
	// ============================================================
	void
	test_ranges_for_each()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 10);
		map.emplace(2, 20);
		map.emplace(3, 30);

		int sum = 0;
		std::ranges::for_each(map, [&sum](auto& kv) { sum += kv.second; });

		AGE_ASSERT(sum == 60);
	}

	// ============================================================
	// 50. ranges - std::ranges::any_of / all_of / none_of
	// ============================================================
	void
	test_ranges_predicates()
	{
		age::unordered_map<uint64, int> map;
		map.emplace(1, 10);
		map.emplace(2, 20);
		map.emplace(3, 30);

		AGE_ASSERT(std::ranges::any_of(map, [](auto& kv) { return kv.second == 20; }));
		AGE_ASSERT(std::ranges::all_of(map, [](auto& kv) { return kv.second > 0; }));
		AGE_ASSERT(std::ranges::none_of(map, [](auto& kv) { return kv.second < 0; }));
	}

	// ============================================================
	// 51. ranges - construct from range (views::iota + transform)
	// ============================================================
	void
	test_ranges_construct_from_range()
	{
		auto rg = std::views::iota(uint64{ 0 }, uint64{ 10 })
				| std::views::transform([](uint64 i) {
					  return std::pair<const uint64, int>{ i, static_cast<int>(i * 100) };
				  });

		age::unordered_map<uint64, int> map(std::from_range, rg);

		AGE_ASSERT(map.size() == 10);
		for (uint64 i = 0; i < 10; ++i)
			AGE_ASSERT(map[i] == static_cast<int>(i * 100));
	}

	// ============================================================
	// 52. ranges - views::take / views::drop
	// ============================================================
	void
	test_ranges_take_drop()
	{
		age::unordered_map<uint64, int> map;
		for (uint64 i = 0; i < 20; ++i)
			map.emplace(i, static_cast<int>(i));

		// take first 5 from iteration order
		uint32 count = 0;
		for (auto& [k, v] : map | std::views::take(5))
			++count;

		AGE_ASSERT(count == 5);

		// drop first 15
		uint32 count2 = 0;
		for (auto& [k, v] : map | std::views::drop(15))
			++count2;

		AGE_ASSERT(count2 == 5);
	}

	// ============================================================
	// run all
	// ============================================================
	void
	run_test()
	{
		test_default_constructor();
		test_emplace();
		test_insert();
		test_try_emplace();
		test_operator_bracket();
		test_find();
		test_at();
		test_contains();
		test_erase();
		test_clear();
		test_size_empty();
		test_iterator();
		test_iterator_deref();
		test_copy_constructor();
		test_move_constructor();
		test_copy_assignment();
		test_move_assignment();
		test_reserve();
		test_resize_stress();
		test_non_trivial_type();
		test_non_trivial_copy_move();
		test_hash_collision();
		test_erase_reinsert();
		test_iterator_after_erase();
		test_empty_map();
		test_initializer_list();
		test_range_constructor();
		test_block_count();
		test_accessors();
		test_large_scale();
		test_real_collision();
		test_collision_erase_reinsert();
		test_nested_map();
		test_vector_of_map();
		test_map_of_vector();
		test_deep_nesting();
		test_stress_insert_erase_cycle();
		test_stress_sequential();
		test_stress_collision_resize();
		test_stress_interleaved();
		test_stress_non_trivial_lifecycle();
		test_stress_copy_move();
		test_ranges_filter();
		test_ranges_transform();
		test_ranges_keys_values();
		test_ranges_chained();
		test_ranges_count_if();
		test_ranges_find_if();
		test_ranges_for_each();
		test_ranges_predicates();
		test_ranges_construct_from_range();
		test_ranges_take_drop();
	}
}	 // namespace age_test::data_structure::unordered_map

namespace test::unordered_map_constexpr
{
	struct constexpr_hash
	{
		constexpr uint64
		operator()(uint64 key) const noexcept
		{
			// FNV-1a
			uint64 h = 0xcbf29ce484222325ull;
			for (int i = 0; i < 8; ++i)
			{
				h	 ^= key & 0xff;
				h	 *= 0x100000001b3ull;
				key >>= 8;
			}
			return h;
		}
	};

	consteval bool
	test_basic()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		if (map.size() != 3) return false;
		if (map.empty()) return false;

		if (map.find(1)->second != 100) return false;
		if (map.find(2)->second != 200) return false;
		if (map.find(3)->second != 300) return false;
		if (map.find(999) != map.end()) return false;

		return true;
	}

	consteval bool
	test_duplicate()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		auto [it1, ok1] = map.emplace(1, 100);
		if (!ok1) return false;

		auto [it2, ok2] = map.emplace(1, 999);
		if (ok2) return false;
		if (it2->second != 100) return false;
		if (map.size() != 1) return false;

		return true;
	}

	consteval bool
	test_try_emplace()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		auto [it1, ok1] = map.try_emplace(1, 100);
		if (!ok1) return false;

		auto [it2, ok2] = map.try_emplace(1, 999);
		if (ok2) return false;
		if (it2->second != 100) return false;

		return true;
	}

	consteval bool
	test_operator_bracket()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		map[1] = 100;
		map[2] = 200;

		if (map[1] != 100) return false;
		if (map[2] != 200) return false;
		if (map.size() != 2) return false;

		map[1] = 999;
		if (map[1] != 999) return false;

		return true;
	}

	consteval bool
	test_erase()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		if (map.erase(2) != 1) return false;
		if (map.size() != 2) return false;
		if (map.contains(2)) return false;

		if (map.erase(999) != 0) return false;

		return true;
	}

	consteval bool
	test_clear()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		map.emplace(1, 100);
		map.emplace(2, 200);

		map.clear();

		if (!map.empty()) return false;
		if (map.find(1) != map.end()) return false;

		map.emplace(3, 300);
		if (map.size() != 1) return false;

		return true;
	}

	consteval bool
	test_iterator()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		map.emplace(1, 100);
		map.emplace(2, 200);
		map.emplace(3, 300);

		int sum	  = 0;
		int count = 0;
		for (auto it = map.begin(); it != map.end(); ++it)
		{
			sum += it->second;
			++count;
		}

		if (count != 3) return false;
		if (sum != 600) return false;

		return true;
	}

	consteval bool
	test_copy()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;
		map.emplace(1, 100);
		map.emplace(2, 200);

		auto copy = map;

		if (copy.size() != 2) return false;
		if (copy.find(1)->second != 100) return false;
		if (copy.find(2)->second != 200) return false;

		return true;
	}

	consteval bool
	test_move()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;
		map.emplace(1, 100);
		map.emplace(2, 200);

		auto moved = std::move(map);

		if (moved.size() != 2) return false;
		if (map.empty() == false) return false;

		return true;
	}

	consteval bool
	test_resize()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;

		for (uint64 i = 0; i < 100; ++i)
			map.emplace(i, static_cast<int>(i * 5));

		if (map.size() != 100) return false;

		for (uint64 i = 0; i < 100; ++i)
		{
			if (map.find(i)->second != static_cast<int>(i * 5))
				return false;
		}

		return true;
	}

	consteval bool
	test_reserve()
	{
		age::unordered_map<uint64, int, constexpr_hash> map;
		map.emplace(1, 100);

		map.reserve(256);

		if (map.size() != 1) return false;
		if (map.find(1)->second != 100) return false;

		return true;
	}

	// compile-time validation
	static_assert(test_basic());
	static_assert(test_duplicate());
	static_assert(test_try_emplace());
	static_assert(test_operator_bracket());
	static_assert(test_erase());
	static_assert(test_clear());
	static_assert(test_iterator());
	static_assert(test_copy());
	static_assert(test_move());
	static_assert(test_resize());
	static_assert(test_reserve());

}	 // namespace test::unordered_map_constexpr