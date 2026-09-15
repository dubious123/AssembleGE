#pragma once

namespace age::util
{
	// t_children_of(uint32) returns ranges of child indices
	template <std::ranges::input_range t_root_range, typename t_children_of, typename t_visit>
	requires(std::is_integral_v<std::ranges::range_value_t<t_root_range>>
			 and std::ranges::bidirectional_range<std::invoke_result_t<t_children_of&, uint32>>
			 and std::is_integral_v<std::ranges::range_value_t<std::invoke_result_t<t_children_of&, uint32>>>)
	void
	for_each_preorder(t_root_range&& root_range, t_children_of&& children_of, t_visit&& visit)
	{
		auto stack_vec = age::vector<uint32>{};
		for (auto root_idx : root_range)
		{
			stack_vec.emplace_back(root_idx);
			while (stack_vec.empty() is_false)
			{
				c_auto idx = stack_vec.back();
				stack_vec.pop_back();
				visit(idx);

				for (auto child_idx : children_of(idx) | std::views::reverse)
				{
					stack_vec.emplace_back(child_idx);
				}
			}
		}
	}
}	 // namespace age::util