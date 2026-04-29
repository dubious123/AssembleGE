#pragma once
#include "age.hpp"

namespace age::asset::detail
{
	template <e::kind e_kind>
	std::string_view
	extract_asset_name(const std::array<char, config::max_asset_path_len>& full_path) noexcept
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
		else if constexpr (e_kind == e::kind::mesh_baked)
		{
			if (full_name.ends_with(config::mesh_baked_asset_tag))
			{
				full_name.remove_suffix(std::size(config::mesh_baked_asset_tag) - 1);
			}
		}

		return full_name;
	}
}	 // namespace age::asset::detail

namespace age::asset
{
	template <e::kind e_kind>
	std::array<char, config::max_asset_display_name_len>
	get_display_name(const std::array<char, config::max_asset_path_len>& path) noexcept
	{
		auto res = std::array<char, config::max_asset_display_name_len>{};
		auto sv	 = std::string_view{ path.data() };

		if (sv.ends_with(config::asset_extension))
		{
			sv.remove_suffix(sizeof(config::asset_extension) - 1);
		}

		if (sv.ends_with(get_asset_tag<e_kind>()))
		{
			sv.remove_suffix(sizeof(get_asset_tag<e_kind>()) - 1);
		}

		c_auto slash_pos = sv.find_last_of("/\\");
		if (slash_pos != std::string_view::npos)
		{
			sv.remove_prefix(slash_pos + 1);
		}

		c_auto len = sv.size() < config::max_asset_display_name_len - 1
					   ? sv.size()
					   : config::max_asset_display_name_len - 1;

		std::memcpy(res.data(), sv.data(), len);
		// res[len] = '\0'; redundant

		return res;
	}
}	 // namespace age::asset