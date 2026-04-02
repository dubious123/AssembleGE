#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui::detail
{
	// FNV-1a
	FORCE_INLINE constexpr uint64
	hash(const char* p_str) noexcept
	{
		uint64 res = g::fnv1a_offset_basis;
		while (*p_str != 0)
		{
			res ^= static_cast<uint64>(*p_str++);
			res *= g::fnv1a_prime;
		}
		return res;
	}

	FORCE_INLINE constexpr uint64
	hash(uint64 val) noexcept
	{
		uint64 res = g::fnv1a_offset_basis;
		for (int i = 0; i < 8; ++i)
		{
			res	 ^= val & 0xff;
			res	 *= g::fnv1a_prime;
			val >>= 8;
		}
		return res;
	}

	FORCE_INLINE constexpr uint64
	hash_combine(uint64 parent, uint64 child) noexcept
	{
		return (parent ^ child) * g::fnv1a_prime;
	}
}	 // namespace age::ui::detail

namespace age::ui
{
	FORCE_INLINE t_hash
	new_id() noexcept
	{
		auto&  scope   = g::id_stack.back();
		c_auto hash_id = detail::hash_combine(scope.hash_id, detail::hash(++scope.counter));

		g::id_stack.emplace_back(id_scope{ .hash_id = hash_id, .counter = 0 });

		return hash_id;
	}

	FORCE_INLINE id_ctx
	id_begin() noexcept
	{
		new_id();
		return id_ctx{};
	}

	t_hash
	hash(const char* p_str) noexcept
	{
		return detail::hash(p_str);
	}
}	 // namespace age::ui