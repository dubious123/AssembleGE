#pragma once

namespace age::inline data_structure
{
	template <typename t_data, std::size_t static_count = std::dynamic_extent>
	struct sparse_vector
	{
		struct bucket
		{
			using storage_t = std::conditional_t<(sizeof(t_data) >= sizeof(std::size_t)), t_data, std::size_t>;

			alignas(alignof(storage_t))
				std::byte storage[sizeof(storage_t)];

			FORCE_INLINE
			bucket() noexcept = default;

			template <typename... t>
			FORCE_INLINE
			bucket(t&&... arg) noexcept
			{
				std::construct_at(reinterpret_cast<t_data*>(storage), std::forward<t>(arg)...);
			}

			FORCE_INLINE
			bucket(const t_data& data) noexcept
			{
				std::construct_at(reinterpret_cast<t_data*>(storage), data);
			}

			FORCE_INLINE t_data&
			data() noexcept
			{
				return *std::launder(reinterpret_cast<t_data*>(storage));
			}

			FORCE_INLINE const t_data&
			data() const noexcept
			{
				return *std::launder(reinterpret_cast<const t_data*>(storage));
			}

			FORCE_INLINE std::size_t&
			next_hole_idx() noexcept
			{
				return *std::launder(reinterpret_cast<std::size_t*>(storage));
			}
		};

		static constexpr bool is_static = static_count != std::dynamic_extent;
		using t_bucket_container		= std::conditional_t<is_static, std::array<bucket, static_count>, data_structure::vector<bucket>>;

		std::size_t hole_idx = 0;

		std::size_t hole_count = 0;

		t_bucket_container container = {};

		constexpr sparse_vector() noexcept requires(is_static)
		{
			hole_count = static_count;

			for (auto&& [idx, elem] : container | std::views::enumerate)
			{
				elem.next_hole_idx() = idx + 1;
			}
		}

		constexpr sparse_vector() noexcept requires(is_static == false)
		= default;

		FORCE_INLINE constexpr std::size_t
		capacity() const noexcept
		{
			return container.size();
		}

		FORCE_INLINE
		std::size_t
		size() const noexcept
		{
			return container.size() - hole_count;
		}

		template <typename... t>
		std::size_t
		emplace_back(t&&... arg) noexcept
		{
			auto res = 0uz;

			if constexpr (is_static)
			{
				AGE_ASSERT(hole_count > 0);

				res				   = hole_idx;
				auto hole_idx_temp = container[hole_idx].next_hole_idx();
				std::construct_at(reinterpret_cast<t_data*>(&container[hole_idx].storage), std::forward<t>(arg)...);
				hole_idx = hole_idx_temp;
				--hole_count;

				return res;
			}
			else
			{
				if (hole_count == 0)
				{
					res = container.size();
					container.emplace_back(std::forward<t>(arg)...);
				}
				else
				{
					res				   = hole_idx;
					auto hole_idx_temp = container[hole_idx].next_hole_idx();
					std::construct_at(reinterpret_cast<t_data*>(&container[hole_idx].storage), std::forward<t>(arg)...);
					hole_idx = hole_idx_temp;
					--hole_count;
				}

				return res;
			}
		}

		void
		remove(std::size_t idx) noexcept
		{
			if constexpr (not std::is_trivially_destructible_v<t_data>)
			{
				std::destroy_at(&container[idx].data());
			}

			container[idx].next_hole_idx() = hole_idx;
			hole_idx					   = idx;
			++hole_count;
		}

		FORCE_INLINE t_data&
		operator[](std::size_t idx) noexcept
		{
			return container[idx].data();
		}

		FORCE_INLINE const t_data&
		operator[](std::size_t idx) const noexcept
		{
			return container[idx].data();
		}

		void
		debug_validate()
		{
			if constexpr (age::config::debug_mode)
			{
				AGE_ASSERT(hole_count <= container.size());
				AGE_ASSERT(hole_count + size() == container.size());

				{
					auto visited = std::vector<uint8>(container.size(), 0);

					for (auto curr = hole_idx;
						 auto _ : std::views::iota(0ul, hole_count))
					{
						AGE_ASSERT(curr < container.size());

						AGE_ASSERT(visited[curr] == 0);

						visited[curr] = 1;

						curr = container[curr].next_hole_idx();
					}
				}
			}
		}
	};
}	 // namespace age::inline data_structure