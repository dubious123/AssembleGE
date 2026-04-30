#pragma once

namespace age::inline data_structure
{
	namespace detail
	{
		template <typename t_size = uint32>
		struct offset_pool_hole
		{
			t_size offset;
			t_size size;
		};
	}	 // namespace detail

	template <typename t_size = uint32, typename t_allocator = std::allocator<detail::offset_pool_hole<t_size>>>
	struct offset_pool
	{
		void
		set_capacity(t_size new_cap) noexcept
		{
			AGE_ASSERT(new_cap >= cap);
			AGE_ASSERT(new_cap >= size);

			cap = new_cap;
		}

		void
		set_size(t_size new_size) noexcept
		{
			AGE_ASSERT(cap >= new_size);

			size = new_size;
		}

		t_size
		allocate(t_size alloc_size) noexcept
		{
			AGE_ASSERT(alloc_size > 0);

			for (auto&& [idx, hole] : hole_vec | std::views::enumerate)
			{
				if (hole.size < alloc_size) { continue; }

				c_auto offset = hole.offset;

				if (hole.size == alloc_size)
				{
					std::rotate(hole_vec.begin() + idx, hole_vec.begin() + idx + 1, hole_vec.end());
					hole_vec.pop_back();
				}
				else
				{
					hole.offset += alloc_size;
					hole.size	-= alloc_size;
				}

				return offset;
			}


			if (size + alloc_size <= cap)
			{
				c_auto offset  = size;
				size		  += alloc_size;
				return offset;
			}

			return age::get_invalid_idx<t_size>();
		}

		t_size
		allocate(std::integral auto alloc_size) noexcept
			requires(meta::is_not_same_v<BARE_OF(alloc_size), t_size>)
		{
			return allocate(static_cast<t_size>(alloc_size));
		}

		void
		free(t_size free_offset, t_size free_size) noexcept
		{
			AGE_ASSERT(free_size > 0);
			AGE_ASSERT(free_offset + free_size <= size);

			for (auto&& [idx, hole] : hole_vec | std::views::enumerate)
			{
				if (hole.offset + hole.size < free_offset) { continue; }

				if (hole.offset + hole.size == free_offset)
				{
					hole.size += free_size;

					if (idx + 1 < hole_vec.size<int64>())
					{
						if (auto& hole_next = hole_vec[idx + 1]; hole_next.offset == hole.offset + hole.size)
						{
							hole.size += hole_next.size;
							std::rotate(hole_vec.begin() + idx + 1, hole_vec.begin() + idx + 2, hole_vec.end());
							hole_vec.pop_back();
						}
					}

					goto goto__return;
				}
				else if (hole.offset == free_offset + free_size)
				{
					hole.offset	 = free_offset;
					hole.size	+= free_size;
					goto goto__return;
				}
				else if (hole.offset > free_offset + free_size)
				{
					hole_vec.emplace_back(free_offset, free_size);
					std::rotate(hole_vec.rbegin(), hole_vec.rbegin() + 1, hole_vec.rbegin() + hole_vec.size() - idx);
					goto goto__return;
				}
				else
				{
					AGE_UNREACHABLE("overlapping free range");
				}
			}

			hole_vec.emplace_back(free_offset, free_size);

		goto__return:
			if (c_auto& hole_back = hole_vec.back(); hole_back.offset + hole_back.size == size)
			{
				size = hole_back.offset;
				hole_vec.pop_back();
			}
		}

		void
		free(std::integral auto free_offset, std::integral auto free_size) noexcept
			requires(meta::is_not_same_v<BARE_OF(free_offset), t_size> or meta::is_not_same_v<BARE_OF(free_size), t_size>)
		{
			free(static_cast<t_size>(free_offset), static_cast<t_size>(free_size));
		}

		void
		clear() noexcept
		{
			cap	 = 0;
			size = 0;
			hole_vec.clear();
		}

		t_size
		capacity() const noexcept
		{
			return cap;
		}

		t_size
		used() const noexcept
		{
			return size;
		}

		t_size
		max_contiguous_free() const noexcept
		{
			auto res = t_size{ cap - size };
			for (c_auto& h : hole_vec)
			{
				res = std::max(h.size, res);
			}

			return res;
		}

	  private:
		t_size													   cap	= {};
		t_size													   size = {};
		age::vector<detail::offset_pool_hole<t_size>, t_allocator> hole_vec;
	};
}	 // namespace age::inline data_structure