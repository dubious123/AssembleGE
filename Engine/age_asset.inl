#pragma once
#include "age.hpp"

namespace age::asset::detail
{
	template <e::kind e_kind>
	std::string_view
	extract_asset_name(c_auto& full_path) noexcept
	{
		auto full_name = std::string_view{ full_path.data() };
		// "font_name.xxx.age_asset" -> "font_name"
		if (full_name.ends_with(config::asset_extension))
		{
			full_name.remove_suffix(std::size(config::asset_extension) - 1);
		}

		if constexpr (e_kind == e::kind::font)
		{
			if (full_name.ends_with(config::font_asset_tag))
			{
				full_name.remove_suffix(std::size(config::font_asset_tag) - 1);
			}
		}

		return full_name;
	}
}	 // namespace age::asset::detail