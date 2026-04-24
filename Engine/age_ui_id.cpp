#include "age_pch.hpp"
#include "age.hpp"

namespace age::ui
{
	t_hash
	new_id() noexcept
	{
		auto&  scope   = g::id_stack.back();
		c_auto hash_id = detail::hash_combine(scope.hash_id, detail::hash(++scope.counter));

		g::id_stack.emplace_back(id_scope{ .hash_id = hash_id, .counter = 0 });

		return hash_id;
	}

	id_ctx
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