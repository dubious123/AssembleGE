#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset::registry
{
	void
	load(const char* root_dir) noexcept
	{
		g::registry_path = std::filesystem::path{ root_dir } / std::string_view{ std::format("{}{}", config::asset_registry_asset_tag, config::asset_extension) };

		if (std::filesystem::exists(g::registry_path) is_false)
		{
			std::filesystem::create_directories(root_dir);
			return;
		}

		auto h_registry = load_from_path(g::registry_path);

		auto buf = h_registry->get_payload_read_buf();

		auto asset_kind_count = buf.read<std::underlying_type_t<e::kind>>();

		for (auto _ : views::loop(asset_kind_count))
		{
			auto&& [asset_kind_name, asset_count] = buf.read<std::array<char, config::max_enum_name_len>, uint32>();
			auto asset_kind						  = e::str_to_enum<e::kind>(asset_kind_name);

			auto& registry_vec	 = g::registry_map[e::to_idx(asset_kind)];
			auto& path_to_handle = g::registry_path_to_handle_map[e::to_idx(asset_kind)];
			registry_vec.reserve(asset_count);
			path_to_handle.reserve(asset_count);
			for (auto _ : views::loop(asset_count))
			{
				auto asset_path = buf.read<std::array<char, config::max_asset_path_len>>();

				auto h_asset = asset::create_entry(asset_kind, asset_path.data());

				registry_vec.emplace_back(h_asset);
				path_to_handle[asset_path] = h_asset;
			}
		}

		AGE_ASSERT(buf.has_remaining() is_false);

		asset::unload(h_registry);
	}

	void
	save() noexcept
	{
		AGE_ASSERT(g::registry_path.empty() is_false);
		AGE_ASSERT(std::filesystem::exists(g::registry_path));

		// todo,

		auto buf = byte_buf{};

		auto asset_kind_count = std::underlying_type_t<e::kind>{ 0 };

		buf.write(asset_kind_count);

		for (auto&& [idx, vec] : g::registry_map | std::views::enumerate)
		{
			if (vec.empty()) { continue; }

			++asset_kind_count;

			buf.write(util::to_fixed_str<config::max_enum_name_len>(e::to_string(static_cast<e::kind>(idx))), vec.size<uint32>());

			for (auto& h_asset : vec)
			{
				buf.write(h_asset.get_path());
			}
		}

		buf.write_at(0, asset_kind_count);

		write_to_file(g::registry_path, get_default_file_header<e::kind::asset_registry>(buf.size()), buf);
	}
}	 // namespace age::asset::registry