#include "age_test_pch.hpp"
#include "age_test.hpp"

namespace age_test::data_structure::byte_buffer
{
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
		AGE_ASSERT(buf.empty());
		AGE_ASSERT(buf.is_empty());
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
	// 4. write + read multiple trivial (tuple read)
	// ============================================================
	void
	test_write_read_multiple()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 1 }, float{ 2.0f }, uint64{ 3 });
		AGE_ASSERT(buf.size() == sizeof(uint32) + sizeof(float) + sizeof(uint64));

		auto [a, b, c] = buf.read<uint32, float, uint64>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2.0f);
		AGE_ASSERT(c == 3);
	}

	// ============================================================
	// 5. write byte size matches sum of sizeofs (no padding)
	// ============================================================
	void
	test_byte_size_no_padding()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 0xFF });
		buf.write(uint64{ 123456789 });

		AGE_ASSERT(buf.size() == sizeof(uint8) + sizeof(uint64));

		c_auto v1 = buf.read<uint8>();
		c_auto v2 = buf.read<uint64>();
		AGE_ASSERT(v1 == 0xFF);
		AGE_ASSERT(v2 == 123456789);
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
		AGE_ASSERT(buf.capacity() == old_cap);
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

		c_auto v = moved.read<uint32>();
		AGE_ASSERT(v == 42);
	}

	// ============================================================
	// 14. copy assignment
	// ============================================================
	void
	test_copy_assignment()
	{
		age::byte_buffer src;
		src.write(uint32{ 1 }, uint32{ 2 });

		age::byte_buffer dst;
		dst.write(uint64{ 99 });

		dst = src;

		AGE_ASSERT(dst.size() == src.size());

		c_auto a = dst.read<uint32>();
		c_auto b = dst.read<uint32>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2);
	}

	// ============================================================
	// 15. move assignment
	// ============================================================
	void
	test_move_assignment()
	{
		age::byte_buffer src;
		src.write(uint32{ 42 }, uint64{ 99 });
		c_auto src_size = src.size();

		age::byte_buffer dst;
		dst.write(uint32{ 1 });

		dst = std::move(src);

		AGE_ASSERT(dst.size() == src_size);
		AGE_ASSERT(src.size() == 0);

		c_auto a = dst.read<uint32>();
		c_auto b = dst.read<uint64>();
		AGE_ASSERT(a == 42);
		AGE_ASSERT(b == 99);
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
		c_auto v = buf.read<uint32>();
		AGE_ASSERT(v == 42);
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
		c_auto v = buf.read<uint32>();
		AGE_ASSERT(v == 42);
	}

	// ============================================================
	// 18. operator[]
	// ============================================================
	void
	test_operator_bracket()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 0xDEADBEEF });

		AGE_ASSERT(buf[0] == std::byte{ 0xEF });
		AGE_ASSERT(buf[1] == std::byte{ 0xBE });
		AGE_ASSERT(buf[2] == std::byte{ 0xAD });
		AGE_ASSERT(buf[3] == std::byte{ 0xDE });
	}

	// ============================================================
	// 19. auto growth
	// ============================================================
	void
	test_auto_growth()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 1000; ++i)
		{
			buf.write(i);
		}

		AGE_ASSERT(buf.size() == 1000 * sizeof(uint32));
		AGE_ASSERT(buf.capacity() >= 1000 * sizeof(uint32));

		for (uint32 i = 0; i < 1000; ++i)
		{
			c_auto v = buf.read<uint32>();
			AGE_ASSERT(v == i);
		}
	}

	// ============================================================
	// 20. mixed types sequential write/read
	// ============================================================
	void
	test_mixed_types()
	{
		age::byte_buffer buf;

		buf.write(uint8{ 0xAB }, uint16{ 0xCDEF }, uint32{ 0x12345678 }, uint64{ 0xDEADBEEF12345678 });

		c_auto a = buf.read<uint8>();
		c_auto b = buf.read<uint16>();
		c_auto c = buf.read<uint32>();
		c_auto d = buf.read<uint64>();

		AGE_ASSERT(a == 0xAB);
		AGE_ASSERT(b == 0xCDEF);
		AGE_ASSERT(c == 0x12345678);
		AGE_ASSERT(d == 0xDEADBEEF12345678);
	}

	// ============================================================
	// 21. write after clear
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
	// 22. multiple read cycles with reset_read
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
	// 23. struct write/read
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
	// 24. data pointer stability after reserve
	// ============================================================
	void
	test_data_pointer_after_reserve()
	{
		age::byte_buffer buf;
		buf.reserve(4096);

		c_auto* ptr_before = buf.data();
		buf.write(uint32{ 1 }, uint64{ 2 }, float{ 3.0f });
		c_auto* ptr_after = buf.data();

		AGE_ASSERT(ptr_before == ptr_after);
	}

	// ============================================================
	// 25. stress - sequential
	// ============================================================
	void
	test_stress_sequential()
	{
		age::byte_buffer buf;

		for (uint32 i = 0; i < 50000; ++i)
		{
			buf.write(i);
		}

		AGE_ASSERT(buf.size() == 50000 * sizeof(uint32));

		for (uint32 i = 0; i < 50000; ++i)
		{
			c_auto v = buf.read<uint32>();
			AGE_ASSERT(v == i);
		}

		AGE_ASSERT(buf.has_remaining() is_false);
	}

	// ============================================================
	// 26. stress - clear cycles
	// ============================================================
	void
	test_stress_clear_cycles()
	{
		age::byte_buffer buf;

		for (uint32 cycle = 0; cycle < 100; ++cycle)
		{
			for (uint32 i = 0; i < 100; ++i)
			{
				buf.write(i);
			}

			for (uint32 i = 0; i < 100; ++i)
			{
				c_auto v = buf.read<uint32>();
				AGE_ASSERT(v == i);
			}

			buf.clear();
		}
	}

	// ============================================================
	// 27. stress - copy under load
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
	// 28. stress - move chain
	// ============================================================
	void
	test_stress_move_chain()
	{
		age::byte_buffer buf1;
		for (uint32 i = 0; i < 500; ++i)
		{
			buf1.write(i);
		}

		auto buf2 = std::move(buf1);
		auto buf3 = std::move(buf2);
		auto buf4 = std::move(buf3);

		for (uint32 i = 0; i < 500; ++i)
		{
			c_auto v = buf4.read<uint32>();
			AGE_ASSERT(v == i);
		}
	}

	// ============================================================
	// 29. get_allocator
	// ============================================================
	void
	test_get_allocator()
	{
		age::byte_buffer buf;
		c_auto			 alloc = buf.get_allocator();
		(void)alloc;
	}

	// ============================================================
	// 30. empty / is_empty
	// ============================================================
	void
	test_empty()
	{
		age::byte_buffer buf;
		AGE_ASSERT(buf.empty());
		AGE_ASSERT(buf.is_empty());

		buf.write(uint32{ 1 });
		AGE_ASSERT(buf.empty() is_false);
		AGE_ASSERT(buf.is_empty() is_false);

		buf.clear();
		AGE_ASSERT(buf.empty());
		AGE_ASSERT(buf.is_empty());
	}

	// ============================================================
	// 31. const access
	// ============================================================
	void
	test_const_access()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 42 });

		c_auto& cbuf = buf;
		AGE_ASSERT(cbuf.size() == sizeof(uint32));
		AGE_ASSERT(cbuf.capacity() >= sizeof(uint32));
		AGE_ASSERT(cbuf.data() is_not_nullptr);
		AGE_ASSERT(cbuf[0] != std::byte{ 0 });
	}

	// ============================================================
	// 32. zero arg write - should be no-op
	// ============================================================
	void
	test_zero_arg_write()
	{
		age::byte_buffer buf;

		buf.write();
		AGE_ASSERT(buf.size() == 0);
	}

	// ============================================================
	// 33. multi write, tuple read
	// ============================================================
	void
	test_multi_write_tuple_read()
	{
		age::byte_buffer buf;

		buf.write(uint32{ 1 }, uint16{ 2 }, uint8{ 3 });

		auto [a, b, c] = buf.read<uint32, uint16, uint8>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2);
		AGE_ASSERT(c == 3);
	}

	// ============================================================
	// 34. read by reference (out parameter form)
	// ============================================================
	void
	test_read_by_reference()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 42 }, uint64{ 999 }, float{ 1.5f });

		uint32 a;
		uint64 b;
		float  c;
		buf.read(a, b, c);

		AGE_ASSERT(a == 42);
		AGE_ASSERT(b == 999);
		AGE_ASSERT(c == 1.5f);
	}

	// ============================================================
	// 35. read into C array
	// ============================================================
	void
	test_read_c_array()
	{
		age::byte_buffer buf;
		for (uint32 i = 0; i < 8; ++i)
		{
			buf.write(i * 11u);
		}

		uint32 arr[8];
		buf.read(arr);

		for (uint32 i = 0; i < 8; ++i)
		{
			AGE_ASSERT(arr[i] == i * 11u);
		}
	}

	// ============================================================
	// 36. read raw memory (single)
	// ============================================================
	void
	test_read_raw_single()
	{
		age::byte_buffer buf;
		buf.write(uint64{ 0xDEADBEEFCAFEBABE });

		alignas(uint64) std::byte storage[sizeof(uint64)];
		auto*					  p = buf.read<uint64>(storage);

		AGE_ASSERT(*p == 0xDEADBEEFCAFEBABE);
	}

	// ============================================================
	// 37. read raw memory (n elements)
	// ============================================================
	void
	test_read_raw_array()
	{
		age::byte_buffer buf;
		for (uint32 i = 0; i < 16; ++i)
		{
			buf.write(i);
		}

		alignas(uint32) std::byte storage[sizeof(uint32) * 16];
		auto*					  p = buf.read<uint32>(storage, 16u);

		for (uint32 i = 0; i < 16; ++i)
		{
			AGE_ASSERT(p[i] == i);
		}
	}

	// ============================================================
	// 38. write_bytes
	// ============================================================
	void
	test_write_bytes()
	{
		age::byte_buffer buf;

		c_auto vals = std::array<uint32, 4>{ 11, 22, 33, 44 };
		buf.write_bytes(vals.data(), uint32{ sizeof(vals) });

		AGE_ASSERT(buf.size() == sizeof(vals));

		for (auto v : vals)
		{
			c_auto r = buf.read<uint32>();
			AGE_ASSERT(r == v);
		}
	}

	// ============================================================
	// 39. write_bytes triggers growth
	// ============================================================
	void
	test_write_bytes_growth()
	{
		age::byte_buffer buf;

		auto big = age::vector<uint32>{};
		big.resize(2000);
		for (uint32 i = 0; i < 2000; ++i)
		{
			big[i] = i;
		}

		buf.write_bytes(big.data(), uint32{ sizeof(uint32) * 2000 });
		AGE_ASSERT(buf.size() == sizeof(uint32) * 2000);

		for (uint32 i = 0; i < 2000; ++i)
		{
			c_auto v = buf.read<uint32>();
			AGE_ASSERT(v == i);
		}
	}

	// ============================================================
	// 40. move_write_pos - backward (truncate)
	// ============================================================
	void
	test_move_write_pos_backward()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 1 }, uint32{ 2 }, uint32{ 3 });
		AGE_ASSERT(buf.size() == 3 * sizeof(uint32));

		buf.move_write_pos(sizeof(uint32));
		AGE_ASSERT(buf.size() == sizeof(uint32));

		c_auto v = buf.read<uint32>();
		AGE_ASSERT(v == 1);
	}

	// ============================================================
	// 41. move_write_pos - forward (reserve region)
	// ============================================================
	void
	test_move_write_pos_forward()
	{
		age::byte_buffer buf;
		buf.reserve(256);

		// reserve a region without writing
		buf.move_write_pos(64);
		AGE_ASSERT(buf.size() == 64);

		// fill the region via write_at
		buf.write_at(0, uint32{ 100 }, uint32{ 200 });

		c_auto a = buf.read<uint32>();
		c_auto b = buf.read<uint32>();
		AGE_ASSERT(a == 100);
		AGE_ASSERT(b == 200);
	}

	// ============================================================
	// 42. write_at - overwrite mid-buffer without changing write_pos
	// ============================================================
	void
	test_write_at()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 1 }, uint32{ 2 }, uint32{ 3 });
		c_auto size_before = buf.size();

		buf.write_at(sizeof(uint32), uint32{ 99 });
		AGE_ASSERT(buf.size() == size_before);

		c_auto a = buf.read<uint32>();
		c_auto b = buf.read<uint32>();
		c_auto c = buf.read<uint32>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 99);
		AGE_ASSERT(c == 3);
	}

	// ============================================================
	// 43. skip_read
	// ============================================================
	void
	test_skip_read()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 1 }, uint32{ 2 }, uint32{ 3 }, uint32{ 4 });

		buf.skip_read(sizeof(uint32) * 2);

		c_auto v3 = buf.read<uint32>();
		c_auto v4 = buf.read<uint32>();
		AGE_ASSERT(v3 == 3);
		AGE_ASSERT(v4 == 4);
	}

	// ============================================================
	// 44. write/read header pattern (move_write_pos + write_at)
	// ============================================================
	void
	test_header_pattern()
	{
		// pattern: reserve header slot, write payload, fill header with payload size
		age::byte_buffer buf;
		buf.reserve(64);

		c_auto header_pos = buf.size();
		buf.move_write_pos(header_pos + sizeof(uint32));	// reserve header slot

		buf.write(uint32{ 10 }, uint32{ 20 }, uint32{ 30 });
		c_auto payload_size = static_cast<uint32>(buf.size() - header_pos - sizeof(uint32));

		buf.write_at(header_pos, payload_size);

		c_auto sz = buf.read<uint32>();
		AGE_ASSERT(sz == 3 * sizeof(uint32));

		c_auto a = buf.read<uint32>();
		c_auto b = buf.read<uint32>();
		c_auto c = buf.read<uint32>();
		AGE_ASSERT(a == 10);
		AGE_ASSERT(b == 20);
		AGE_ASSERT(c == 30);
	}

	// ============================================================
	// read_byte_buffer (read_buf) tests
	// ============================================================

	// ============================================================
	// 45. read_buf - default constructor
	// ============================================================
	void
	test_read_buf_default()
	{
		age::read_byte_buffer rb;

		AGE_ASSERT(rb.size() == 0);
		AGE_ASSERT(rb.read_amount() == 0);
		AGE_ASSERT(rb.data() == nullptr);
		AGE_ASSERT(rb.has_remaining() is_false);
		AGE_ASSERT(rb.empty());
	}

	// ============================================================
	// 46. read_buf - from raw pointer
	// ============================================================
	void
	test_read_buf_from_ptr()
	{
		uint32				  src[] = { 11, 22, 33 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		AGE_ASSERT(rb.size() == sizeof(src));
		AGE_ASSERT(rb.read_amount() == 0);
		AGE_ASSERT(rb.has_remaining());

		c_auto a = rb.read<uint32>();
		c_auto b = rb.read<uint32>();
		c_auto c = rb.read<uint32>();
		AGE_ASSERT(a == 11);
		AGE_ASSERT(b == 22);
		AGE_ASSERT(c == 33);
		AGE_ASSERT(rb.has_remaining() is_false);
	}

	// ============================================================
	// 47. read_buf - from byte_buffer (ctor)
	// ============================================================
	void
	test_read_buf_from_byte_buffer()
	{
		age::byte_buffer buf;
		buf.write(uint32{ 1 }, uint64{ 2 }, float{ 3.0f });

		age::read_byte_buffer rb{ buf };

		AGE_ASSERT(rb.size() == buf.size());

		c_auto a = rb.read<uint32>();
		c_auto b = rb.read<uint64>();
		c_auto c = rb.read<float>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2);
		AGE_ASSERT(c == 3.0f);
	}

	// ============================================================
	// 48. read_buf - tuple read
	// ============================================================
	void
	test_read_buf_tuple_read()
	{
		uint32				  src[] = { 7, 8, 9 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		auto [a, b, c] = rb.read<uint32, uint32, uint32>();
		AGE_ASSERT(a == 7);
		AGE_ASSERT(b == 8);
		AGE_ASSERT(c == 9);
	}

	// ============================================================
	// 49. read_buf - read by reference
	// ============================================================
	void
	test_read_buf_by_reference()
	{
		age::byte_buffer wb;
		wb.write(uint32{ 42 }, uint64{ 999 }, float{ 1.5f });

		age::read_byte_buffer rb{ wb };

		uint32 a;
		uint64 b;
		float  c;
		rb.read(a, b, c);

		AGE_ASSERT(a == 42);
		AGE_ASSERT(b == 999);
		AGE_ASSERT(c == 1.5f);
	}

	// ============================================================
	// 50. read_buf - C array
	// ============================================================
	void
	test_read_buf_c_array()
	{
		uint32				  src[6] = { 0, 1, 2, 3, 4, 5 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		uint32 dst[6];
		rb.read(dst);

		for (uint32 i = 0; i < 6; ++i)
		{
			AGE_ASSERT(dst[i] == i);
		}
	}

	// ============================================================
	// 51. read_buf - raw memory single
	// ============================================================
	void
	test_read_buf_raw_single()
	{
		uint64				  src = 0xDEADBEEFCAFEBABE;
		age::read_byte_buffer rb{ &src, sizeof(uint64) };

		alignas(uint64) std::byte storage[sizeof(uint64)];
		auto*					  p = rb.read<uint64>(storage);

		AGE_ASSERT(*p == 0xDEADBEEFCAFEBABE);
	}

	// ============================================================
	// 52. read_buf - raw memory n elements
	// ============================================================
	void
	test_read_buf_raw_array()
	{
		uint32				  src[8] = { 100, 101, 102, 103, 104, 105, 106, 107 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		alignas(uint32) std::byte storage[sizeof(src)];
		auto*					  p = rb.read<uint32>(storage, 8u);

		for (uint32 i = 0; i < 8; ++i)
		{
			AGE_ASSERT(p[i] == 100 + i);
		}
	}

	// ============================================================
	// 53. read_buf - skip_read
	// ============================================================
	void
	test_read_buf_skip_read()
	{
		uint32				  src[] = { 1, 2, 3, 4 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		rb.skip_read(sizeof(uint32) * 2);

		c_auto a = rb.read<uint32>();
		c_auto b = rb.read<uint32>();
		AGE_ASSERT(a == 3);
		AGE_ASSERT(b == 4);
	}

	// ============================================================
	// 54. read_buf - reset_read
	// ============================================================
	void
	test_read_buf_reset_read()
	{
		uint32				  src[] = { 10, 20, 30 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		c_auto a1 = rb.read<uint32>();
		c_auto a2 = rb.read<uint32>();
		AGE_ASSERT(a1 == 10);
		AGE_ASSERT(a2 == 20);

		rb.reset_read();
		AGE_ASSERT(rb.read_amount() == 0);

		c_auto b1 = rb.read<uint32>();
		c_auto b2 = rb.read<uint32>();
		c_auto b3 = rb.read<uint32>();
		AGE_ASSERT(b1 == 10);
		AGE_ASSERT(b2 == 20);
		AGE_ASSERT(b3 == 30);
	}

	// ============================================================
	// 55. read_buf - operator[]
	// ============================================================
	void
	test_read_buf_operator_bracket()
	{
		uint32				  src = 0xDEADBEEF;
		age::read_byte_buffer rb{ &src, sizeof(uint32) };

		AGE_ASSERT(rb[0] == std::byte{ 0xEF });
		AGE_ASSERT(rb[1] == std::byte{ 0xBE });
		AGE_ASSERT(rb[2] == std::byte{ 0xAD });
		AGE_ASSERT(rb[3] == std::byte{ 0xDE });
	}

	// ============================================================
	// 56. read_buf - empty / is_empty
	// ============================================================
	void
	test_read_buf_empty()
	{
		age::read_byte_buffer rb_empty;
		AGE_ASSERT(rb_empty.empty());
		AGE_ASSERT(rb_empty.is_empty());

		uint32				  src = 1;
		age::read_byte_buffer rb{ &src, sizeof(uint32) };
		AGE_ASSERT(rb.empty() is_false);
		AGE_ASSERT(rb.is_empty() is_false);
	}

	// ============================================================
	// 57. read_buf - has_remaining transitions
	// ============================================================
	void
	test_read_buf_has_remaining()
	{
		uint32				  src[] = { 1, 2 };
		age::read_byte_buffer rb{ src, sizeof(src) };

		AGE_ASSERT(rb.has_remaining());
		(void)rb.read<uint32>();
		AGE_ASSERT(rb.has_remaining());
		(void)rb.read<uint32>();
		AGE_ASSERT(rb.has_remaining() is_false);
	}

	// ============================================================
	// 58. write+read round trip via read_buf
	// ============================================================
	void
	test_round_trip_via_read_buf()
	{
		age::byte_buffer wb;
		wb.write(uint32{ 1 }, uint64{ 2 }, float{ 3.0f }, uint16{ 4 });

		age::read_byte_buffer rb{ wb.data(), wb.size() };

		auto [a, b, c, d] = rb.read<uint32, uint64, float, uint16>();
		AGE_ASSERT(a == 1);
		AGE_ASSERT(b == 2);
		AGE_ASSERT(c == 3.0f);
		AGE_ASSERT(d == 4);
	}

	// ============================================================
	void
	run_test()
	{
		test_default_constructor();
		test_gen_reserved();
		test_write_read_single();
		test_write_read_multiple();
		test_byte_size_no_padding();
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
		test_auto_growth();
		test_mixed_types();
		test_write_after_clear();
		test_multiple_read_cycles();
		test_struct();
		test_data_pointer_after_reserve();
		test_stress_sequential();
		test_stress_clear_cycles();
		test_stress_copy();
		test_stress_move_chain();
		test_get_allocator();
		test_empty();
		test_const_access();
		test_zero_arg_write();
		test_multi_write_tuple_read();
		test_read_by_reference();
		test_read_c_array();
		test_read_raw_single();
		test_read_raw_array();
		test_write_bytes();
		test_write_bytes_growth();
		test_move_write_pos_backward();
		test_move_write_pos_forward();
		test_write_at();
		test_skip_read();
		test_header_pattern();

		test_read_buf_default();
		test_read_buf_from_ptr();
		test_read_buf_from_byte_buffer();
		test_read_buf_tuple_read();
		test_read_buf_by_reference();
		test_read_buf_c_array();
		test_read_buf_raw_single();
		test_read_buf_raw_array();
		test_read_buf_skip_read();
		test_read_buf_reset_read();
		test_read_buf_operator_bracket();
		test_read_buf_empty();
		test_read_buf_has_remaining();
		test_round_trip_via_read_buf();
	}
}	 // namespace age_test::data_structure::byte_buffer