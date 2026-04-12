#include "age_test_pch.hpp"
#include "age_test.hpp"

namespace age_test::data_structure::byte_buffer
{
	// ============================================================
	// custom serializable type for cx_custom_value tests
	// ============================================================
	struct custom_string
	{
		using size_type = std::size_t;

		age::vector<char> chars;

		custom_string() = default;

		custom_string(const char* str) : chars(str, str + std::strlen(str)) { }

		size_type
		byte_size() const
		{ return sizeof(uint32) + chars.size(); }

		void
		write_to(void* p_base, size_type& offset) const
		{
			auto*  p   = static_cast<std::byte*>(p_base) + offset;
			c_auto len = static_cast<uint32>(chars.size());
			std::memcpy(p, &len, sizeof(uint32));
			std::memcpy(p + sizeof(uint32), chars.data(), len);
			offset += sizeof(uint32) + len;
		}

		static custom_string
		read_from(void* p_base, size_type& offset)
		{
			auto*  p = static_cast<std::byte*>(p_base) + offset;
			uint32 len;
			std::memcpy(&len, p, sizeof(uint32));

			custom_string result;
			result.chars.resize(len);
			std::memcpy(result.chars.data(), p + sizeof(uint32), len);
			offset += sizeof(uint32) + len;
			return result;
		}

		bool
		operator==(const custom_string& other) const
		{
			if (chars.size() != other.chars.size()) { return false; }
			return std::memcmp(chars.data(), other.chars.data(), chars.size()) == 0;
		}
	};

	// ============================================================
	// 1. default constructor
	// ============================================================
	void
	test_default_constructor()
	{
		age::byte_buffer buf;

		AGE_ASSERT(buf.size() == 0);
		AGE_ASSERT(buf.capacity() == 0);
		AGE_ASSERT(buf.read_amount() == 0);
		AGE_ASSERT(buf.data() == nullptr);
		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 2. gen_reserved
	// ============================================================
	void
	test_gen_reserved()
	{
		auto buf = age::byte_buffer<>::gen_reserved(256);

		AGE_ASSERT(buf.capacity() == 256);
		AGE_ASSERT(buf.size() == 0);
		AGE_ASSERT(buf.data() is_not_nullptr);
		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 3. write + read single trivial
	// ============================================================
	void
	test_write_read_single()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 42 });
		AGE_ASSERT(buf.size() == sizeof(uint32));
		AGE_ASSERT(buf.has_remaining());

		c_auto val = buf.read<uint32>();
		AGE_ASSERT(val == 42);
		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 4. write + read multiple trivial
	// ============================================================
	void
	test_write_read_multiple()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 1 }, float{ 2.0f }, uint64{ 3 });

		auto [a, b, c] = buf.read<uint32, float, uint64>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2.0f);
		AGE_ASSERT(c == 3);
	}

	// ============================================================
	// 5. write + read with alignment
	// ============================================================
	void
	test_alignment()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 0xFF });
		buf.write(uint64{ 123456789 });

		c_auto v1 = buf.read<uint8>();
		c_auto v2 = buf.read<uint64>();

		AGE_ASSERT(v1 == 0xFF);
		AGE_ASSERT(v2 == 123456789);

		// size should include alignment padding
		AGE_ASSERT(buf.size() > sizeof(uint8) + sizeof(uint64));
	}

	// ============================================================
	// 6. write + read vector types
	// ============================================================
	void
	test_vector_types()
	{
		age::byte_buffer buf;

		c_auto v2 = float2{ 1.0f, 2.0f };
		c_auto v3 = float3{ 3.0f, 4.0f, 5.0f };
		c_auto v4 = float4{ 6.0f, 7.0f, 8.0f, 9.0f };

		buf.write(v2, v3, v4);

		c_auto r2 = buf.read<float2>();
		c_auto r3 = buf.read<float3>();
		c_auto r4 = buf.read<float4>();

		AGE_ASSERT(r2.x == 1.0f);
		AGE_ASSERT(r2.y == 2.0f);
		AGE_ASSERT(r3.x == 3.0f);
		AGE_ASSERT(r3.y == 4.0f);
		AGE_ASSERT(r3.z == 5.0f);
		AGE_ASSERT(r4.x == 6.0f);
		AGE_ASSERT(r4.y == 7.0f);
		AGE_ASSERT(r4.z == 8.0f);
		AGE_ASSERT(r4.w == 9.0f);
	}

	// ============================================================
	// 7. clear
	// ============================================================
	void
	test_clear()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 1 }, uint32{ 2 }, uint32{ 3 });
		c_auto old_cap = buf.capacity();

		buf.clear();
		AGE_ASSERT(buf.size() == 0);
		AGE_ASSERT(buf.read_amount() == 0);
		AGE_ASSERT(buf.has_remaining() is_false);
		AGE_ASSERT(buf.capacity() == old_cap);	  // capacity preserved
	}

	// ============================================================
	// 8. reset_read
	// ============================================================
	void
	test_reset_read()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 42 }, uint32{ 99 });

		c_auto v1 = buf.read<uint32>();
		AGE_ASSERT(v1 == 42);
		AGE_ASSERT(buf.read_amount() > 0);

		buf.reset_read();
		AGE_ASSERT(buf.read_amount() == 0);

		// re-read from start
		c_auto v1_again = buf.read<uint32>();
		c_auto v2		= buf.read<uint32>();
		AGE_ASSERT(v1_again == 42);
		AGE_ASSERT(v2 == 99);
	}

	// ============================================================
	// 9. reserve
	// ============================================================
	void
	test_reserve()
	{
		age::byte_buffer buf;

		buf.reserve(1024);
		AGE_ASSERT(buf.capacity() >= 1024);
		AGE_ASSERT(buf.size() == 0);

		// write should not trigger realloc
		buf.write(uint32{ 1 });
		AGE_ASSERT(buf.capacity() >= 1024);
	}

	// ============================================================
	// 10. reserve preserves data
	// ============================================================
	void
	test_reserve_preserves_data()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 42 }, uint64{ 999 });
		c_auto old_size = buf.size();

		buf.reserve(4096);
		AGE_ASSERT(buf.size() == old_size);

		c_auto v1 = buf.read<uint32>();
		c_auto v2 = buf.read<uint64>();
		AGE_ASSERT(v1 == 42);
		AGE_ASSERT(v2 == 999);
	}

	// ============================================================
	// 11. resize
	// ============================================================
	void
	test_resize()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 10 });
		buf.resize(2048);
		AGE_ASSERT(buf.capacity() == 2048);

		c_auto val = buf.read<uint32>();
		AGE_ASSERT(val == 10);
	}

	// ============================================================
	// 12. copy constructor
	// ============================================================
	void
	test_copy_constructor()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 1 }, uint64{ 2 }, float{ 3.0f });

		auto copy = buf;

		AGE_ASSERT(copy.size() == buf.size());
		AGE_ASSERT(copy.capacity() == buf.capacity());
		AGE_ASSERT(copy.data() != buf.data());

		c_auto a = copy.read<uint32>();
		c_auto b = copy.read<uint64>();
		c_auto c = copy.read<float>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2);
		AGE_ASSERT(c == 3.0f);
	}

	// ============================================================
	// 13. move constructor
	// ============================================================
	void
	test_move_constructor()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 42 });
		c_auto old_size = buf.size();

		auto moved = std::move(buf);

		AGE_ASSERT(moved.size() == old_size);
		AGE_ASSERT(buf.size() == 0);
		AGE_ASSERT(buf.data() == nullptr);

		c_auto val = moved.read<uint32>();
		AGE_ASSERT(val == 42);
	}

	// ============================================================
	// 14. copy assignment
	// ============================================================
	void
	test_copy_assignment()
	{
		age::byte_buffer a;
		a.write(uint32{ 100 }, uint32{ 200 });

		age::byte_buffer b;
		b.write(uint64{ 999 });

		b = a;

		AGE_ASSERT(b.size() == a.size());

		c_auto v1 = b.read<uint32>();
		c_auto v2 = b.read<uint32>();
		AGE_ASSERT(v1 == 100);
		AGE_ASSERT(v2 == 200);
	}

	// ============================================================
	// 15. move assignment
	// ============================================================
	void
	test_move_assignment()
	{
		age::byte_buffer a;
		a.write(uint32{ 77 });

		age::byte_buffer b;
		b.write(uint64{ 999 });

		b = std::move(a);

		AGE_ASSERT(b.size() == sizeof(uint32));
		AGE_ASSERT(a.size() == 0);
		AGE_ASSERT(a.data() == nullptr);

		c_auto val = b.read<uint32>();
		AGE_ASSERT(val == 77);
	}

	// ============================================================
	// 16. self copy assignment
	// ============================================================
	void
	test_self_copy_assignment()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 42 });

		buf = buf;

		AGE_ASSERT(buf.size() == sizeof(uint32));
		c_auto val = buf.read<uint32>();
		AGE_ASSERT(val == 42);
	}

	// ============================================================
	// 17. self move assignment
	// ============================================================
	void
	test_self_move_assignment()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 42 });

		buf = std::move(buf);

		AGE_ASSERT(buf.size() == sizeof(uint32));
		c_auto val = buf.read<uint32>();
		AGE_ASSERT(val == 42);
	}

	// ============================================================
	// 18. operator[]
	// ============================================================
	void
	test_operator_bracket()
	{
		age::byte_buffer buf;
		buf.write(uint8{ 0xAB });

		AGE_ASSERT(static_cast<uint8>(buf[0]) == 0xAB);
	}

	// ============================================================
	// 19. calc_write_size - type only
	// ============================================================
	void
	test_calc_write_size_type_only()
	{
		age::byte_buffer buf;

		c_auto size_a = buf.calc_write_size<uint32>();
		AGE_ASSERT(size_a == sizeof(uint32));

		c_auto size_b = buf.calc_write_size<uint8, uint64>();
		AGE_ASSERT(size_b > sizeof(uint8) + sizeof(uint64));	// includes alignment padding
	}

	// ============================================================
	// 20. calc_write_size - instance
	// ============================================================
	void
	test_calc_write_size_instance()
	{
		age::byte_buffer buf;

		uint32 a = 1;
		float  b = 2.0f;

		c_auto predicted = buf.calc_write_size(a, b);
		buf.write(a, b);
		AGE_ASSERT(buf.size() == predicted);
	}

	// ============================================================
	// 21. auto growth
	// ============================================================
	void
	test_auto_growth()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 1000; ++i)
		{
			buf.write(i);
		}

		AGE_ASSERT(buf.size() == sizeof(uint32) * 1000);

		for (uint32 i = 0; i < 1000; ++i)
		{
			c_auto val = buf.read<uint32>();
			AGE_ASSERT(val == i);
		}

		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 22. mixed types sequential write/read
	// ============================================================
	void
	test_mixed_types()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 1 });
		buf.write(uint16{ 2 });
		buf.write(uint32{ 3 });
		buf.write(uint64{ 4 });
		buf.write(float{ 5.0f });
		buf.write(double{ 6.0 });

		AGE_ASSERT(buf.read<uint8>() == 1);
		AGE_ASSERT(buf.read<uint16>() == 2);
		AGE_ASSERT(buf.read<uint32>() == 3);
		AGE_ASSERT(buf.read<uint64>() == 4);
		AGE_ASSERT(buf.read<float>() == 5.0f);
		AGE_ASSERT(buf.read<double>() == 6.0);
	}

	// ============================================================
	// 23. write after clear - reuse buffer
	// ============================================================
	void
	test_write_after_clear()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 1 }, uint32{ 2 });
		buf.clear();

		buf.write(uint64{ 99 });
		AGE_ASSERT(buf.size() == sizeof(uint64));

		c_auto val = buf.read<uint64>();
		AGE_ASSERT(val == 99);
	}

	// ============================================================
	// 24. multiple read cycles with reset_read
	// ============================================================
	void
	test_multiple_read_cycles()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 10 }, uint32{ 20 }, uint32{ 30 });

		for (int cycle = 0; cycle < 3; ++cycle)
		{
			buf.reset_read();
			AGE_ASSERT(buf.read<uint32>() == 10);
			AGE_ASSERT(buf.read<uint32>() == 20);
			AGE_ASSERT(buf.read<uint32>() == 30);
			AGE_ASSERT(buf.has_remaining() is_false);
		}
	}

	// ============================================================
	// 25. struct write/read
	// ============================================================
	void
	test_struct()
	{
		struct test_data
		{
			uint32 a;
			float  b;
			uint64 c;
		};

		age::byte_buffer buf;

		c_auto src = test_data{ 42, 3.14f, 999 };
		buf.write(src);

		c_auto dst = buf.read<test_data>();
		AGE_ASSERT(dst.a == 42);
		AGE_ASSERT(dst.b == 3.14f);
		AGE_ASSERT(dst.c == 999);
	}

	// ============================================================
	// 26. custom value type - write/read
	// ============================================================
	void
	test_custom_value()
	{
		age::byte_buffer buf;

		c_auto src = custom_string{ "hello age" };
		buf.write(src);

		c_auto dst = buf.read<custom_string>();
		AGE_ASSERT(dst == src);
	}

	// ============================================================
	// 27. custom value mixed with trivial
	// ============================================================
	void
	test_custom_mixed()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 42 }, custom_string{ "test" }, float{ 3.14f });

		c_auto a = buf.read<uint32>();
		c_auto b = buf.read<custom_string>();
		c_auto c = buf.read<float>();

		AGE_ASSERT(a == 42);
		AGE_ASSERT(b == custom_string{ "test" });
		AGE_ASSERT(c == 3.14f);
	}

	// ============================================================
	// 28. multiple custom values
	// ============================================================
	void
	test_custom_multiple()
	{
		age::byte_buffer buf;

		buf.write(custom_string{ "aaa" }, custom_string{ "bbbb" }, custom_string{ "ccccc" });

		c_auto a = buf.read<custom_string>();
		c_auto b = buf.read<custom_string>();
		c_auto c = buf.read<custom_string>();

		AGE_ASSERT(a == custom_string{ "aaa" });
		AGE_ASSERT(b == custom_string{ "bbbb" });
		AGE_ASSERT(c == custom_string{ "ccccc" });
	}

	// ============================================================
	// 29. data pointer stability after reserve
	// ============================================================
	void
	test_data_pointer_after_reserve()
	{
		age::byte_buffer buf;
		buf.reserve(64);

		auto* p1 = buf.data();
		buf.write(uint32{ 1 });

		// no realloc expected
		AGE_ASSERT(buf.data() == p1);
	}

	// ============================================================
	// 30. stress - large sequential
	// ============================================================
	void
	test_stress_sequential()
	{
		age::byte_buffer buf;

		constexpr uint64 N = 50000;

		for (uint64 i = 0; i < N; ++i)
		{
			buf.write(i);
		}

		AGE_ASSERT(buf.size() == sizeof(uint64) * N);

		for (uint64 i = 0; i < N; ++i)
		{
			c_auto val = buf.read<uint64>();
			AGE_ASSERT(val == i);
		}

		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 31. stress - repeated clear + write cycles
	// ============================================================
	void
	test_stress_clear_cycles()
	{
		age::byte_buffer buf;

		for (uint32 cycle = 0; cycle < 100; ++cycle)
		{
			buf.clear();

			for (uint32 i = 0; i < 100; ++i)
			{
				buf.write(i + cycle);
			}

			for (uint32 i = 0; i < 100; ++i)
			{
				c_auto val = buf.read<uint32>();
				AGE_ASSERT(val == i + cycle);
			}
		}
	}

	// ============================================================
	// 32. stress - mixed alignment patterns
	// ============================================================
	void
	test_stress_mixed_alignment()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 500; ++i)
		{
			buf.write(uint8{ static_cast<uint8>(i & 0xFF) });
			buf.write(uint64{ i * 13ull });
			buf.write(uint16{ static_cast<uint16>(i & 0xFFFF) });
			buf.write(float{ static_cast<float>(i) });
		}

		for (uint32 i = 0; i < 500; ++i)
		{
			c_auto a = buf.read<uint8>();
			c_auto b = buf.read<uint64>();
			c_auto c = buf.read<uint16>();
			c_auto d = buf.read<float>();

			AGE_ASSERT(a == static_cast<uint8>(i & 0xFF));
			AGE_ASSERT(b == i * 13ull);
			AGE_ASSERT(c == static_cast<uint16>(i & 0xFFFF));
			AGE_ASSERT(d == static_cast<float>(i));
		}
	}

	// ============================================================
	// 33. stress - copy under load
	// ============================================================
	void
	test_stress_copy()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 1000; ++i)
		{
			buf.write(i);
		}

		auto copy = buf;

		for (uint32 i = 0; i < 1000; ++i)
		{
			c_auto v1 = buf.read<uint32>();
			c_auto v2 = copy.read<uint32>();
			AGE_ASSERT(v1 == v2);
			AGE_ASSERT(v1 == i);
		}
	}

	// ============================================================
	// 34. stress - move chain
	// ============================================================
	void
	test_stress_move_chain()
	{
		age::byte_buffer a;
		for (uint32 i = 0; i < 1000; ++i)
		{
			a.write(i);
		}

		auto b = std::move(a);
		AGE_ASSERT(a.empty() or a.size() == 0);

		auto c = std::move(b);
		AGE_ASSERT(b.empty() or b.size() == 0);

		age::byte_buffer d;
		d = std::move(c);
		AGE_ASSERT(c.empty() or c.size() == 0);

		for (uint32 i = 0; i < 1000; ++i)
		{
			c_auto val = d.read<uint32>();
			AGE_ASSERT(val == i);
		}
	}

	// ============================================================
	// 35. get_allocator
	// ============================================================
	void
	test_get_allocator()
	{
		age::byte_buffer buf;
		auto			 a = buf.get_allocator();
		(void)a;	// compiles
	}

	// ============================================================
	// 36. calc_write_size_at
	// ============================================================
	void
	test_calc_write_size_at()
	{
		age::byte_buffer buf;

		// offset 0 - no padding
		c_auto s1 = buf.calc_write_size_at<uint32>(0);
		AGE_ASSERT(s1 == sizeof(uint32));

		// offset 1 - needs alignment padding for uint32
		c_auto s2 = buf.calc_write_size_at<uint32>(1);
		AGE_ASSERT(s2 > sizeof(uint32));

		// multi type
		c_auto s3 = buf.calc_write_size_at<uint8, uint64>(0);
		c_auto s4 = buf.calc_write_size_at<uint8, uint64>(1);
		AGE_ASSERT(s3 == 16);
		AGE_ASSERT(s4 == 15);
	}

	// ============================================================
	// 37. empty
	// ============================================================
	void
	test_empty()
	{
		age::byte_buffer buf;
		AGE_ASSERT(buf.empty());

		buf.write(uint32{ 1 });
		AGE_ASSERT(buf.empty() is_false);

		buf.clear();
		AGE_ASSERT(buf.empty());
	}

	// ============================================================
	// 38. const access
	// ============================================================
	void
	test_const_access()
	{
		age::byte_buffer buf;
		buf.write(uint8{ 0xAB }, uint32{ 42 });

		const auto& cbuf = buf;

		AGE_ASSERT(cbuf.size() == buf.size());
		AGE_ASSERT(cbuf.capacity() == buf.capacity());
		AGE_ASSERT(cbuf.read_amount() == buf.read_amount());
		AGE_ASSERT(cbuf.has_remaining());
		AGE_ASSERT(cbuf.data() == buf.data());
		AGE_ASSERT(static_cast<uint8>(cbuf[0]) == 0xAB);
		AGE_ASSERT(cbuf.empty() is_false);

		c_auto predicted = cbuf.calc_write_size<uint32>();
		AGE_ASSERT(predicted == sizeof(uint32));
	}

	// ============================================================
	// 39. zero arg write
	// ============================================================
	void
	test_zero_arg_write()
	{
		age::byte_buffer buf;
		buf.write();
		AGE_ASSERT(buf.size() == 0);
		AGE_ASSERT(buf.empty());
	}

	// ============================================================
	// 40. custom value calc_write_size prediction
	// ============================================================
	void
	test_custom_calc_write_size()
	{
		age::byte_buffer buf;

		c_auto s1 = custom_string{ "hello" };
		c_auto s2 = custom_string{ "world!!" };

		c_auto predicted = buf.calc_write_size(s1, s2);
		buf.write(s1, s2);
		AGE_ASSERT(buf.size() == predicted);
	}

	// ============================================================
	// 41. custom value empty string
	// ============================================================
	void
	test_custom_empty_string()
	{
		age::byte_buffer buf;

		c_auto empty = custom_string{ "" };
		buf.write(empty);

		c_auto result = buf.read<custom_string>();
		AGE_ASSERT(result == empty);
		AGE_ASSERT(result.chars.size() == 0);
	}

	// ============================================================
	// 42. single write call with mixed alignment
	// ============================================================
	void
	test_single_write_mixed_alignment()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 1 }, uint16{ 2 }, uint32{ 3 }, uint64{ 4 }, uint8{ 5 }, uint64{ 6 });

		AGE_ASSERT(buf.read<uint8>() == 1);
		AGE_ASSERT(buf.read<uint16>() == 2);
		AGE_ASSERT(buf.read<uint32>() == 3);
		AGE_ASSERT(buf.read<uint64>() == 4);
		AGE_ASSERT(buf.read<uint8>() == 5);
		AGE_ASSERT(buf.read<uint64>() == 6);
	}

	// ============================================================
	// 43. worst case padding pattern
	// ============================================================
	void
	test_worst_case_padding()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 200; ++i)
		{
			buf.write(uint8{ static_cast<uint8>(i) });
			buf.write(uint64{ i * 7ull });
		}

		for (uint32 i = 0; i < 200; ++i)
		{
			c_auto a = buf.read<uint8>();
			c_auto b = buf.read<uint64>();
			AGE_ASSERT(a == static_cast<uint8>(i));
			AGE_ASSERT(b == i * 7ull);
		}
	}

	// ============================================================
	// 44. multi write, tuple read
	// ============================================================
	void
	test_multi_write_tuple_read()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 1 });
		buf.write(uint32{ 2 });
		buf.write(uint64{ 3 });

		auto [a, b, c] = buf.read<uint8, uint32, uint64>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2);
		AGE_ASSERT(c == 3);
	}

	// ============================================================
	// 45. single write, individual reads
	// ============================================================
	void
	test_single_write_individual_reads()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 10 }, uint16{ 20 }, uint32{ 30 }, uint64{ 40 }, float{ 50.0f }, double{ 60.0 });

		AGE_ASSERT(buf.read<uint8>() == 10);
		AGE_ASSERT(buf.read<uint16>() == 20);
		AGE_ASSERT(buf.read<uint32>() == 30);
		AGE_ASSERT(buf.read<uint64>() == 40);
		AGE_ASSERT(buf.read<float>() == 50.0f);
		AGE_ASSERT(buf.read<double>() == 60.0);
	}

	// ============================================================
	// 46. mixed alignment structs
	// ============================================================
	void
	test_mixed_structs()
	{
		struct alignas(1) small_t
		{
			uint8 a;
		};

		struct alignas(4) medium_t
		{
			uint32 a;
			float  b;
		};

		struct alignas(8) large_t
		{
			uint64 a;
			double b;
		};

		age::byte_buffer buf;

		buf.write(small_t{ 1 }, large_t{ 2, 3.0 }, small_t{ 4 }, medium_t{ 5, 6.0f }, large_t{ 7, 8.0 });

		c_auto s1 = buf.read<small_t>();
		c_auto l1 = buf.read<large_t>();
		c_auto s2 = buf.read<small_t>();
		c_auto m1 = buf.read<medium_t>();
		c_auto l2 = buf.read<large_t>();

		AGE_ASSERT(s1.a == 1);
		AGE_ASSERT(l1.a == 2);
		AGE_ASSERT(l1.b == 3.0);
		AGE_ASSERT(s2.a == 4);
		AGE_ASSERT(m1.a == 5);
		AGE_ASSERT(m1.b == 6.0f);
		AGE_ASSERT(l2.a == 7);
		AGE_ASSERT(l2.b == 8.0);
	}

	// ============================================================
	// 47. calc_write_size matches actual write - mixed alignment
	// ============================================================
	void
	test_calc_matches_write_mixed()
	{
		age::byte_buffer buf;

		uint8  a = 1;
		uint64 b = 2;
		uint16 c = 3;
		uint64 d = 4;
		uint8  e = 5;

		c_auto predicted = buf.calc_write_size(a, b, c, d, e);
		buf.write(a, b, c, d, e);
		AGE_ASSERT(buf.size() == predicted);
	}

	// ============================================================
	// 48. stress - alternating 1-byte and 8-byte in single write
	// ============================================================
	void
	test_stress_alternating_alignment()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 1000; ++i)
		{
			buf.write(
				uint8{ static_cast<uint8>(i & 0xFF) },
				uint64{ i * 11ull },
				uint8{ static_cast<uint8>((i + 1) & 0xFF) },
				uint64{ i * 13ull });
		}

		for (uint32 i = 0; i < 1000; ++i)
		{
			c_auto a = buf.read<uint8>();
			c_auto b = buf.read<uint64>();
			c_auto c = buf.read<uint8>();
			c_auto d = buf.read<uint64>();

			AGE_ASSERT(a == static_cast<uint8>(i & 0xFF));
			AGE_ASSERT(b == i * 11ull);
			AGE_ASSERT(c == static_cast<uint8>((i + 1) & 0xFF));
			AGE_ASSERT(d == i * 13ull);
		}

		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 49. custom value - copy buffer
	// ============================================================
	void
	test_custom_copy_buffer()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 1 }, custom_string{ "hello" }, uint64{ 2 });

		auto copy = buf;

		AGE_ASSERT(copy.read<uint32>() == 1);
		AGE_ASSERT(copy.read<custom_string>() == custom_string{ "hello" });
		AGE_ASSERT(copy.read<uint64>() == 2);
	}

	// ============================================================
	// 50. custom value - reset_read and re-read
	// ============================================================
	void
	test_custom_reset_read()
	{
		age::byte_buffer buf;
		buf.write(custom_string{ "abc" }, uint32{ 42 });

		c_auto s1 = buf.read<custom_string>();
		AGE_ASSERT(s1 == custom_string{ "abc" });

		buf.reset_read();

		c_auto s2 = buf.read<custom_string>();
		c_auto v  = buf.read<uint32>();
		AGE_ASSERT(s2 == custom_string{ "abc" });
		AGE_ASSERT(v == 42);
	}

	// ============================================================
	// 51. custom value - clear and rewrite
	// ============================================================
	void
	test_custom_clear_rewrite()
	{
		age::byte_buffer buf;
		buf.write(custom_string{ "first" });
		buf.clear();

		buf.write(custom_string{ "second" }, uint32{ 99 });

		c_auto s = buf.read<custom_string>();
		c_auto v = buf.read<uint32>();
		AGE_ASSERT(s == custom_string{ "second" });
		AGE_ASSERT(v == 99);
	}

	// ============================================================
	// 52. custom value - tuple read
	// ============================================================
	void
	test_custom_tuple_read()
	{
		age::byte_buffer buf;
		buf.write(uint8{ 1 }, custom_string{ "tpl" }, uint64{ 2 });

		auto [a, b, c] = buf.read<uint8, custom_string, uint64>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == custom_string{ "tpl" });
		AGE_ASSERT(c == 2);
	}

	// ============================================================
	// 53. custom value - move buffer
	// ============================================================
	void
	test_custom_move_buffer()
	{
		age::byte_buffer buf;
		buf.write(custom_string{ "move" }, uint32{ 7 });

		auto moved = std::move(buf);
		AGE_ASSERT(buf.size() == 0);

		c_auto s = moved.read<custom_string>();
		c_auto v = moved.read<uint32>();
		AGE_ASSERT(s == custom_string{ "move" });
		AGE_ASSERT(v == 7);
	}

	// ============================================================
	// 54. custom value - stress
	// ============================================================
	void
	test_custom_stress()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 500; ++i)
		{
			buf.write(uint32{ i }, custom_string{ std::to_string(i).c_str() });
		}

		for (uint32 i = 0; i < 500; ++i)
		{
			c_auto v = buf.read<uint32>();
			c_auto s = buf.read<custom_string>();
			AGE_ASSERT(v == i);
			AGE_ASSERT(s == custom_string{ std::to_string(i).c_str() });
		}

		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 55. custom value - varying length
	// ============================================================
	void
	test_custom_varying_length()
	{
		age::byte_buffer buf;

		buf.write(custom_string{ "" });
		buf.write(custom_string{ "a" });
		buf.write(custom_string{ "abcdefghijklmnopqrstuvwxyz" });
		buf.write(uint32{ 42 });

		AGE_ASSERT(buf.read<custom_string>() == custom_string{ "" });
		AGE_ASSERT(buf.read<custom_string>() == custom_string{ "a" });
		AGE_ASSERT(buf.read<custom_string>() == custom_string{ "abcdefghijklmnopqrstuvwxyz" });
		AGE_ASSERT(buf.read<uint32>() == 42);
	}

	// ============================================================
	// run all
	// ============================================================
	void
	run_test()
	{
		test_default_constructor();
		test_gen_reserved();
		test_write_read_single();
		test_write_read_multiple();
		test_alignment();
		test_vector_types();
		test_clear();
		test_reset_read();
		test_reserve();
		test_reserve_preserves_data();
		test_resize();
		test_copy_constructor();
		test_move_constructor();
		test_copy_assignment();
		test_move_assignment();
		test_self_copy_assignment();
		test_self_move_assignment();
		test_operator_bracket();
		test_calc_write_size_type_only();
		test_calc_write_size_instance();
		test_auto_growth();
		test_mixed_types();
		test_write_after_clear();
		test_multiple_read_cycles();
		test_struct();
		test_custom_value();
		test_custom_mixed();
		test_custom_multiple();
		test_data_pointer_after_reserve();
		test_stress_sequential();
		test_stress_clear_cycles();
		test_stress_mixed_alignment();
		test_stress_copy();
		test_stress_move_chain();
		test_get_allocator();
		test_calc_write_size_at();
		test_empty();
		test_const_access();
		test_zero_arg_write();
		test_custom_calc_write_size();
		test_custom_empty_string();
		test_single_write_mixed_alignment();
		test_worst_case_padding();
		test_multi_write_tuple_read();
		test_single_write_individual_reads();
		test_mixed_structs();
		test_calc_matches_write_mixed();
		test_stress_alternating_alignment();
		test_custom_copy_buffer();
		test_custom_reset_read();
		test_custom_clear_rewrite();
		test_custom_tuple_read();
		test_custom_move_buffer();
		test_custom_stress();
		test_custom_varying_length();
	}
}	 // namespace age_test::data_structure::byte_buffer
