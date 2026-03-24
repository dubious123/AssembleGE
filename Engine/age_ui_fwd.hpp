#pragma once
#include "age.hpp"

namespace age::ui::e
{
	AGE_DEFINE_ENUM(shape_kind, uint32,
					rect,
					circle);

	AGE_DEFINE_ENUM(brush_kind, uint32,
					color);
}	 // namespace age::ui::e

namespace age::ui
{
	struct id_ctx
	{
		FORCE_INLINE constexpr explicit
		operator bool() const
		{
			return true;
		}

		FORCE_INLINE ~id_ctx() noexcept;
	};


}	 // namespace age::ui

namespace age::ui
{
	struct render_data
	{
	};

	struct element
	{
		uint64		hash_id;
		uint32		child_count;
		uint32		element_state_idx;
		render_data data;
	};

	struct element_state
	{
		float2 size;
		float2 pivot_pos;
		float2 pivot_uv;
		float  rotation;
	};
}	 // namespace age::ui

namespace age::ui::g
{
	inline constexpr uint64 fnv1a_offset_basis = 0xcbf29ce484222325ull;
	inline constexpr uint64 fnv1a_prime		   = 0x100000001b3ull;

	inline age::vector<uint64> id_stack;

	inline age::vector<element>						 element_vec;
	inline age::unordered_map<uint64, element_state> element_state_map;
}	 // namespace age::ui::g