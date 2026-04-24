#pragma once
#include "age.hpp"

namespace age::asset::g
{
	inline constexpr uint32 uv_set_max = 4;

	inline constexpr auto mashlet_max_vertex_count	  = 64ul;
	inline constexpr auto mashlet_max_primitive_count = 126ul;

	inline constexpr auto asset_header_magic = uint32{ 'AGEA' };
}	 // namespace age::asset::g

namespace age::asset::g
{
	inline auto asset_data_vec = age::stable_dense_vector<asset::data>::gen_reserved(2);
}

// version 2
namespace age::asset
{
	inline handle
	create_entry(e::kind asset_kind, std::string_view asset_path) noexcept;

	template <e::kind e_kind>
	handle
	create_entry(std::string_view asset_path) noexcept;

	AGE_DEFINE_ASSET_KIND(font);
}	 // namespace age::asset

namespace age::asset
{
	bool
	validate(const file_header&) noexcept;

	handle
	load_from_file(std::string_view file_name) noexcept;

	handle
	load_from_path(std::string_view file_path) noexcept;

	handle
	load_from_path(const std::filesystem::path& full_path) noexcept;

	handle
	load_from_blob(std::string_view file_path, const file_header& header, auto& blob) noexcept;

	void
	write_to_file(std::string_view file_name, const file_header&, const auto& asset_data) noexcept;

	void
	write_to_file(const std::filesystem::path& file_path, const file_header& header, const auto& asset_data) noexcept;

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

	asset_header&
	get_asset_header(handle _) noexcept;

	uint16
	calc_unicode_count(e::font_charset_flag flag) noexcept;
}	 // namespace age::asset::font

namespace age::asset
{
	template <e::kind e_asset_kind>
	constexpr file_header
	get_default_file_header(uint64 payload_size) noexcept
	{
		return file_header{
			.magic		   = g::asset_header_magic,
			.header_size   = sizeof(file_header),
			.file_size	   = payload_size + sizeof(file_header),
			.version_major = config::version_major,
			.version_minor = config::version_minor,
			.asset_kind	   = e_asset_kind,
		};
	}
}	 // namespace age::asset

namespace age::asset
{
	FORCE_INLINE data*
	handle::operator->() const noexcept
	{
		return &g::asset_data_vec[id];
	}
}	 // namespace age::asset

namespace age::asset::detail
{
	constexpr std::align_val_t
	get_alignment(asset::e::kind asset_kind);
}