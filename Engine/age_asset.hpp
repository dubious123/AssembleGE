#pragma once
#include "age.hpp"

namespace age::asset
{
	inline bool
	validate_header(const e::kind, file_header& header) noexcept;

	template <e::kind>
	bool
	validate_header(const file_header& header) noexcept;

	aligned_byte_buf
	read_asset_file(std::string_view file_path) noexcept;

	aligned_byte_buf
	read_asset_file(const std::array<char, config::max_asset_path_len>& full_path) noexcept;

	void
	write_asset_file(const std::filesystem::path& file_path, const file_header& header, const void* p_src) noexcept;

	inline handle
	create_entry(e::kind asset_kind, std::string_view asset_path) noexcept;

	template <e::kind e_kind>
	handle
	create_entry(std::string_view asset_path) noexcept;

	inline void
	destroy_entry(handle&) noexcept;

	template <e::kind e_kind>
	void
	destroy_entry(handle&) noexcept;

	AGE_DEFINE_ASSET_KIND(font, mesh_baked);
}	 // namespace age::asset

namespace age::asset
{
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
	void
	load(handle h_font, auto& renderer, e::font_charset_flag flag = e::font_charset_flag::ascii, std::span<uint16> extra_unicode = {}) noexcept;

	handle
	load(std::string_view font_name, auto& renderer, e::font_charset_flag flag = e::font_charset_flag::ascii, std::span<uint16> extra_unicode = {}) noexcept;

	void
	full_unload(handle, auto& renderer) noexcept;
}	 // namespace age::asset::font

namespace age::asset::mesh_baked
{
	void
	cpu_unload(handle h_mesh) noexcept;

	void
	cpu_load(handle h_mesh, const primitive_desc& desc, e::vertex_kind v_kind) noexcept;

	void
	cpu_load(handle h_mesh) noexcept;

	handle
	cpu_load(std::string_view mesh_name, const primitive_desc& desc, e::vertex_kind v_kind) noexcept;

	handle
	cpu_load(std::string_view mesh_name) noexcept;

	void
	gpu_unload(handle h_mesh, auto& renderer) noexcept;

	void
	gpu_load(handle, auto& renderer, const primitive_desc&, e::vertex_kind) noexcept;

	handle
	gpu_load(std::string_view mesh_name, auto& renderer, const primitive_desc&, e::vertex_kind) noexcept;

	void
	gpu_load(handle& h_mesh, auto& renderer) noexcept;

	handle
	gpu_load(std::string_view mesh_name, auto& renderer) noexcept;

	void
	full_unload(handle h_mesh, auto& renderer) noexcept;

	void
	full_unload(std::string_view mesh_name, auto& renderer) noexcept;

	void
	full_load(handle h_mesh, auto& renderer) noexcept;

	handle
	full_load(std::string_view mesh_name, auto& renderer) noexcept;

	void
	add_ref(handle _) noexcept;

	void
	remove_ref(handle _) noexcept;

}	 // namespace age::asset::mesh_baked

namespace age::asset
{
	template <e::kind e_asset_kind>
	constexpr file_header
	get_default_file_header(uint64 payload_size, uint8 blob_alignment_log2 = 4) noexcept
	{
		return file_header{
			.magic				 = g::asset_header_magic,
			.header_size		 = sizeof(file_header),
			.file_size			 = payload_size + sizeof(file_header),
			.version_major		 = config::version_major,
			.version_minor		 = config::version_minor,
			.asset_kind			 = e_asset_kind,
			.blob_alignment_log2 = blob_alignment_log2,
		};
	}
}	 // namespace age::asset
