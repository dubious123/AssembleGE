#pragma once
#include "age.hpp"

namespace age::asset::g
{
	inline constexpr uint32 uv_set_max = 4;

	inline constexpr auto mashlet_thread_count		  = 32ul;
	inline constexpr auto mashlet_max_vertex_count	  = 64ul;
	inline constexpr auto mashlet_max_primitive_count = 126ul;

	inline constexpr auto asset_header_magic = uint32{ 'AGEA' };
}	 // namespace age::asset::g

namespace age::asset::g
{
	inline auto asset_data_vec = age::stable_dense_vector<asset::data>::gen_reserved(2);
}

namespace age::asset
{
	bool
	validate(const file_header&, const std::ifstream&) noexcept;

	asset::handle
	load_from_file(std::string_view file_name) noexcept;

	asset::handle
	load_from_path(std::string_view file_path) noexcept;

	handle
	load_from_blob(std::string_view file_path, const file_header& header, auto& blob) noexcept;

	void
	write_to_file(std::string_view file_path, const file_header&, const auto& asset_data) noexcept;

	void
	unload(asset::handle& h) noexcept;

	void
	deinit() noexcept;
}	 // namespace age::asset

namespace age::asset
{
	mesh_editable
	create_primitive_mesh(const primitive_desc& desc) noexcept;

	void
	calculate_normal(mesh_editable&, const normal_calc_desc&) noexcept;

	void
	calculate_tangent(mesh_editable&, const tangent_calc_desc&) noexcept;
}	 // namespace age::asset

namespace age::asset::font
{
	handle
	load(const std::string_view& font_name, e::font_charset_flag flag, std::span<uint16> extra_unicode_span) noexcept;

	FORCE_INLINE uint16
	calc_unicode_count(e::font_charset_flag flag) noexcept
	{
		auto res = 0;

		if (e::has_all(flag, e::font_charset_flag::ascii))	   // ascii
		{
			res += ('~' - ' ' + 1);
		}

		if (e::has_all(flag, e::font_charset_flag::hangul))	   // hangul
		{
			res += (0xD7A3 - 0xAC00 + 1);
		}

		return static_cast<uint16>(res);
	}
}	 // namespace age::asset::font

#if defined(AGE_EDITOR)
namespace age::asset::editor
{
	scene_data
	load_scene(std::string_view scene_name) noexcept;

	void
	save_scene(const char (&scene_name)[config::max_scene_name_len], std::filesystem::path directory_path, ecs::cx_entity_storage auto&&... ecs_storage) noexcept;

	bool
	validate(const scene_asset_header&) noexcept;
}	 // namespace age::asset::editor
#endif

namespace age::asset
{
	FORCE_INLINE data*
	handle::operator->() const noexcept
	{
		return &g::asset_data_vec[id];
	}

	template <e::kind e_kind>
	decltype(auto)
	data::get_asset_header() noexcept
	{
		if constexpr (e_kind == e::kind::font)
		{
			return *std::start_lifetime_as<font::asset_header>(blob.data());
		}
		else if constexpr (e_kind == e::kind::editor_scene and age::config::is_editor_build)
		{
			AGE_ASSERT(blob.size_bytes() < sizeof(editor::scene_asset_header));

			c_auto& header = *std::start_lifetime_as<const editor::scene_asset_header>(blob.data());

			AGE_ASSERT(editor::validate(header));

			return header;
		}
		else
		{
			AGE_UNREACHABLE();
		}
	}
}	 // namespace age::asset