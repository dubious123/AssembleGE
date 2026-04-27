#pragma once

namespace age
{
	template <typename t>
	struct aligned_allocator
	{
		using value_type							 = t;
		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap			 = std::true_type;
		using is_always_equal						 = std::false_type;

		size_t align = alignof(t);

		constexpr aligned_allocator() noexcept = default;

		constexpr explicit aligned_allocator(size_t a) noexcept
			: align{ a }
		{
			AGE_ASSERT(a >= alignof(t));
			AGE_ASSERT(a >= 1 && (a & (a - 1)) == 0);
		}

		template <typename u>
		constexpr aligned_allocator(const aligned_allocator<u>& other) noexcept
			: align{ other.align }
		{
			AGE_ASSERT(align >= alignof(t));
		}

		FORCE_INLINE t*
		allocate(size_t n) noexcept
		{
			return static_cast<t*>(
				::operator new(n * sizeof(t), std::align_val_t{ align }, std::nothrow));
		}

		FORCE_INLINE void
		deallocate(t* p, size_t size) noexcept
		{
			::operator delete(p, size, std::align_val_t{ align });
		}

		FORCE_INLINE void
		deallocate(t* p) noexcept
		{
			::operator delete(p, std::align_val_t{ align });
		}

		bool
		operator==(const aligned_allocator&) const noexcept = default;
	};

	using aligned_byte_allocator = aligned_allocator<std::byte>;
}	 // namespace age
