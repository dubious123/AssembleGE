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
		return parent ^ (child * g::fnv1a_prime);
	}
}	 // namespace age::ui::detail

namespace age::ui
{
	FORCE_INLINE id_ctx
	push_id(const char* p_str) noexcept
	{
		auto val = detail::hash_combine(g::id_stack.back(), detail::hash(p_str));
		g::id_stack.emplace_back(val);
		return id_ctx{};
	}

	FORCE_INLINE id_ctx
	push_id(uint64 i) noexcept
	{
		auto val = detail::hash_combine(g::id_stack.back(), detail::hash(i));
		g::id_stack.emplace_back(val);
		return id_ctx{};
	}

	FORCE_INLINE uint64
	new_id(const char* p_str) noexcept
	{
		return detail::hash_combine(g::id_stack.back(), detail::hash(p_str));
	}

	FORCE_INLINE uint64
	new_id(uint64 i) noexcept
	{
		return detail::hash_combine(g::id_stack.back(), detail::hash(i));
	}
}	 // namespace age::ui