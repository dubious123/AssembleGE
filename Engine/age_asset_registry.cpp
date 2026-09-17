#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset::registry
{
	void
	load(std::string_view dir) noexcept
	{
		g::registry_path = fs::join(dir, std::format("{}{}", config::asset_registry_asset_tag, config::asset_extension));

		if (c_auto registry_full_path = fs::join(asset::get_root_dir(), g::registry_path);
			fs::exists(registry_full_path) is_false)
		{
			if (fs::create_dir(fs::get_parent_path(registry_full_path)) is_false)
			{
				AGE_ASSERT(false, "registry dir create failed, target path : {}", fs::get_parent_path(registry_full_path));
				std::abort();
			}
			return;
		}

		if (auto file_data = asset::read_asset_file(g::registry_path);
			file_data.is_valid())
		{
			auto& buf = file_data.buf;
			switch (file_data.header.asset_version)
			{
			case 0u:
			{
				auto asset_kind_count = buf.read<std::underlying_type_t<e::kind>>();

				for (auto _ : views::loop(asset_kind_count))
				{
					auto&& [asset_kind_name, asset_count] = buf.read<age::array<char, config::max_enum_name_len>, uint32>();
					auto asset_kind						  = e::str_to_enum<e::kind>(asset_kind_name);

					auto& registry_vec = g::registry_map[e::to_idx(asset_kind)];
					registry_vec.reserve(asset_count);
					for (auto _ : views::loop(asset_count))
					{
						c_auto asset_path = util::to_fixed_str<config::max_asset_path_len>(fs::normalize_path(asset::to_root_relative(
							buf.read<age::array<char, config::max_asset_path_len>>().data())));

						auto h_asset = asset::find(asset_kind, asset_path);
						if (runtime::is_handle_invalid(h_asset))
						{
							h_asset = create_entry(asset_kind, asset_path);
						}

						registry_vec.emplace_back(h_asset);
					}
				}

				AGE_ASSERT(buf.has_remaining() is_false);
				break;
			}
			// ^^^ path not root relative
			case 1:
			{
				auto asset_kind_count = buf.read<std::underlying_type_t<e::kind>>();

				for (auto _ : views::loop(asset_kind_count))
				{
					auto&& [asset_kind_name, asset_count] = buf.read<age::array<char, config::max_enum_name_len>, uint32>();
					auto asset_kind						  = e::str_to_enum<e::kind>(asset_kind_name);

					auto& registry_vec = g::registry_map[e::to_idx(asset_kind)];
					registry_vec.reserve(asset_count);
					for (auto _ : views::loop(asset_count))
					{
						c_auto asset_path = fs::normalize_path(buf.read<age::array<char, config::max_asset_path_len>>().data());

						auto h_asset = asset::find(asset_kind, asset_path);
						if (runtime::is_handle_invalid(h_asset))
						{
							h_asset = create_entry(asset_kind, asset_path);
						}

						registry_vec.emplace_back(h_asset);
					}
				}

				AGE_ASSERT(buf.has_remaining() is_false);
				break;
			}
			// ^^^ path not normalized
			case config::asset_registry_asset_version:
			{
				auto asset_kind_count = buf.read<std::underlying_type_t<e::kind>>();

				for (auto _ : views::loop(asset_kind_count))
				{
					auto&& [asset_kind_name, asset_count] = buf.read<age::array<char, config::max_enum_name_len>, uint32>();
					auto asset_kind						  = e::str_to_enum<e::kind>(asset_kind_name);

					auto& registry_vec = g::registry_map[e::to_idx(asset_kind)];
					registry_vec.reserve(asset_count);
					for (auto _ : views::loop(asset_count))
					{
						auto asset_path = buf.read<age::array<char, config::max_asset_path_len>>();

						auto h_asset = asset::find(asset_kind, asset_path);
						if (runtime::is_handle_invalid(h_asset))
						{
							h_asset = create_entry(asset_kind, asset_path);
						}

						registry_vec.emplace_back(h_asset);
					}
				}

				AGE_ASSERT(buf.has_remaining() is_false);
				break;
			}
			default:
			{
				AGE_ASSERT(false, "invalid asset version");
				break;
			}
			}

			return;
		}

		AGE_ASSERT(false, "invalid registry file");
	}

	void
	save() noexcept
	{
		AGE_ASSERT(g::registry_path.empty() is_false);

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

		write_asset_file(g::registry_path, get_default_file_header(e::kind::asset_registry, buf.size(), config::asset_registry_asset_version), buf.data());
	}

	void
	register_asset(asset::handle h) noexcept
	{
		c_auto asset_kind = h.get_kind();

		g::registry_map[to_idx(asset_kind)].emplace_back(h);
	}

	void
	register_asset(e::kind e_kind, const char* path) noexcept
	{
		register_asset(create_entry(e_kind, path));
	}

	void
	unregister_asset(asset::handle h) noexcept
	{
		c_auto asset_kind = h.get_kind();

		ranges::erase(g::registry_map[to_idx(asset_kind)], h);
	}

	void
	unregister_asset(e::kind asset_kind, const char* path) noexcept
	{
		c_auto h = find(asset_kind, path);

		if (runtime::is_handle_invalid(h)) { return; }

		ranges::erase(g::registry_map[to_idx(asset_kind)], h);
	}

	bool
	is_registered(asset::handle h) noexcept
	{
		if (runtime::is_handle_invalid(h))
		{
			return false;
		}
		auto& vec = g::registry_map[to_idx(h.get_kind())];
		auto  it  = std::ranges::find(vec, h);
		return it != vec.end();
	}

	std::span<const asset::handle>
	all(e::kind e_kind) noexcept
	{
		return g::registry_map[to_idx(e_kind)];
	}

	void
	clear() noexcept
	{
		if constexpr (config::debug_mode)
		{
			for_each_kind([]<e::kind e_kind> {
				for (auto h : g::registry_map[to_idx(e_kind)])
				{
					AGE_ASSERT(h.is_any_loaded<e_kind>() is_false);
				}
			});
		}

		for_each_kind([]<e::kind e_kind> {
			for (auto h : g::registry_map[to_idx(e_kind)])
			{
				destroy_entry<e_kind>(h);
			}
		});

		for (auto& vec : g::registry_map)
		{
			vec.clear();
		}
	}
}	 // namespace age::asset::registry