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

		if constexpr (e_kind == e_kind)
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

	template <e::kind e_kind>
	handle
	load_common_from_path(const std::array<char, config::max_asset_path_len>& full_path) noexcept
	{
		auto h_asset = find(e_kind, full_path);

		if (AGE_IS_INVALID_ID(h_asset.id))
		{
			h_asset = asset::create_entry<e_kind>(full_path.data());
		}

		return h_asset;
	}

	template <e::kind e_kind>
	handle
	load_common(std::string_view asset_name) noexcept
	{
		if (asset_name.size() == 0)
		{
			return {};
		}

		return load_common_from_path<e_kind>(get_asset_full_path<e_kind>(asset_name));
	}


}	 // namespace age::asset::detail

namespace age::asset
{
	template <e::kind e_kind>
	handle
	create_entry(const std::array<char, config::max_asset_path_len>& asset_path) noexcept
	{
		auto& path_to_handle = g::path_to_handle_map[to_idx(e_kind)];
		if (auto it = path_to_handle.find(asset_path); it != path_to_handle.end())
		{
			return it->second;
		}

		auto& pool = pool_of<e_kind>();

		auto idx = pool.emplace_back(entry<e_kind>{
			.path_id = static_cast<uint32>(
				g::path_vec.emplace_back(asset_path)),
		});

		c_auto h = handle::make<e_kind>(idx);

		path_to_handle[asset_path] = h;

		return h;
	}

	template <e::kind e_kind>
	handle
	create_entry(std::string_view asset_path) noexcept
	{
		AGE_ASSERT(asset_path.size() < config::max_asset_path_len);
		return create_entry<e_kind>(util::to_fixed_str<config::max_asset_path_len>(asset_path));
	}

	template <e::kind e_kind>
	void
	destroy_entry(handle& h_asset) noexcept
	{
		auto& entry = h_asset.get_entry<e_kind>();

		if constexpr (requires { entry.is_loaded(); })
		{
			AGE_ASSERT(entry.is_loaded() is_false);
		}
		else if constexpr (requires { entry.is_cpu_loaded(); })
		{
			AGE_ASSERT(entry.is_cpu_loaded() is_false);
		}
		else if constexpr (requires { entry.is_gpu_loaded(); })
		{
			AGE_ASSERT(entry.is_gpu_loaded() is_false);
		}

		auto& path = g::path_vec[entry.path_id];
		g::path_to_handle_map[to_idx(e_kind)].erase(path);

		g::path_vec.remove(entry.path_id);

		auto& pool = pool_of<e_kind>();

		pool.remove(h_asset.get_idx());

		h_asset.id = age::get_invalid_id<t_asset_id>();
	}

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