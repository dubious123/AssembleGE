#pragma once
#include "age.hpp"

namespace age::asset
{
	inline bool
	validate_header(const e::kind, file_header& header) noexcept;

	template <e::kind>
	bool
	validate_header(const file_header& header) noexcept;

	file_data_aligned
	read_asset_file(std::string_view file_path) noexcept;

	file_data_aligned
	read_asset_file(const age::array<char, config::max_asset_path_len>& full_path) noexcept;

	byte_buf
	read_raw_file(std::string_view full_path) noexcept;

	void
	write_asset_file(const std::filesystem::path& file_path, const file_header& header, const void* p_src) noexcept;

	bool
	write_raw_file(std::string_view full_path, const byte_buf& buf) noexcept;

	inline handle
	create_entry(e::kind asset_kind, std::string_view asset_path) noexcept;

	inline handle
	create_entry(e::kind asset_kind, const age::array<char, config::max_asset_path_len>& asset_path) noexcept;

	template <e::kind e_kind>
	handle
	create_entry(const age::array<char, config::max_asset_path_len>& asset_path) noexcept;

	template <e::kind e_kind>
	handle
	create_entry(std::string_view asset_path) noexcept;

	// Immediately destroys the entry and resets the given handle.
	// Entries are otherwise immortal at runtime (no cleanup path exists), and
	// destruction is not observable from other handles or the registry - any
	// surviving copy silently dangles. Deinit stage only, unless this handle is
	// provably the sole reference to the asset.
	inline void
	destroy_entry(handle&) noexcept;

	template <e::kind e_kind>
	void
	destroy_entry(handle&) noexcept;

	template <e::kind e_kind>
	age::array<char, config::max_asset_display_name_len>
	get_display_name(const age::array<char, config::max_asset_path_len>&) noexcept;

	template <e::kind e_kind>
	consteval const auto&
	get_asset_tag() noexcept;

	handle
	find(e::kind, std::string_view full_path) noexcept;

	handle
	find(e::kind, const age::array<char, config::max_asset_path_len>&) noexcept;

	template <e::kind e_kind>
	constexpr decltype(auto)
	get_asset_full_path(std::string_view asset_name) noexcept;

	template <e::kind e_kind>
	void
	add_ref(handle h) noexcept;

	template <e::kind e_kind>
	void
	remove_ref(handle h) noexcept;

	void
	normalize_asset_path(e::kind e_kind, age::array<char, config::max_asset_path_len>&) noexcept;

	e::asset_path_error_kind
	validate_asset_path(e::kind, asset::handle, const age::array<char, config::max_asset_path_len>& new_path) noexcept;

	// validate path before calling this function
	template <e::kind e_kind>
	bool
	update_asset_path(handle h, const age::array<char, config::max_asset_path_len>& new_valid_path) noexcept;

	std::string_view
	get_path_error_msg(e::asset_path_error_kind _) noexcept;
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
	cpu_load(handle h_mesh, const primitive_desc&, e::vertex_kind v_kind) noexcept;

	void
	cpu_load(handle h_mesh, std::span<const primitive_desc>, e::vertex_kind v_kind) noexcept;

	void
	cpu_load(handle h_mesh) noexcept;

	handle
	cpu_load(std::string_view mesh_name, std::span<const primitive_desc>, e::vertex_kind v_kind) noexcept;

	handle
	cpu_load(std::string_view mesh_name, const primitive_desc&, e::vertex_kind v_kind) noexcept;

	handle
	cpu_load(std::string_view mesh_name) noexcept;

	void
	gpu_unload(handle h_mesh, auto& renderer) noexcept;

	void
	gpu_load(handle, auto& renderer, std::span<const primitive_desc>, e::vertex_kind) noexcept;

	void
	gpu_load(handle, auto& renderer, const primitive_desc&, e::vertex_kind) noexcept;

	handle
	gpu_load(std::string_view mesh_name, auto& renderer, std::span<const primitive_desc>, e::vertex_kind) noexcept;

	handle
	gpu_load(std::string_view mesh_name, auto& renderer, const primitive_desc&, e::vertex_kind) noexcept;

	void
	gpu_load(handle h_mesh, auto& renderer) noexcept;

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
}	 // namespace age::asset::mesh_baked

namespace age::asset::texture
{
	void
	full_unload(handle, auto& renderer) noexcept;

	void
	cpu_unload(handle _) noexcept;

	void
	gpu_unload(handle, auto& renderer) noexcept;

	void
	gpu_load(handle, auto& renderer) noexcept;

	handle
	gpu_load(std::string_view tex_name, auto& renderer) noexcept;

	void
	cpu_load(handle _) noexcept;

	handle
	cpu_load(std::string_view tex_name) noexcept;

	void
	full_load(handle, auto& renderer) noexcept;

	handle
	full_load(std::string_view tex_name, auto& renderer) noexcept;

	bool
	bake(std::span<const char* const> src, std::string_view dst, texture_bake_option) noexcept;
}	 // namespace age::asset::texture

namespace age::asset::material
{
	void
	full_unload(handle, auto& renderer) noexcept;

	void
	load(handle, auto& renderer) noexcept;

	handle
	load(std::string_view mat_name, auto& renderer) noexcept;

	void
	update_texture(asset::handle& h_tex_before, asset::handle h_tex_after) noexcept;

	void
	build(std::string_view mat_path, const material_desc&) noexcept;

	void
	save(handle _) noexcept;
};	  // namespace age::asset::material

namespace age::asset::env_light
{
	void
	full_unload(handle, auto& renderer) noexcept;

	void
	cpu_unload(handle _) noexcept;

	void
	gpu_unload(handle, auto& renderer) noexcept;

	void
	gpu_load(handle, auto& renderer) noexcept;

	handle
	gpu_load(std::string_view name, auto& renderer) noexcept;

	void
	cpu_load(handle _) noexcept;

	handle
	cpu_load(std::string_view name) noexcept;

	void
	full_load(handle, auto& renderer) noexcept;

	handle
	full_load(std::string_view name, auto& renderer) noexcept;

	bool
	bake(const age::array<char, config::max_asset_path_len>& src,
		 const age::array<char, config::max_asset_path_len>& dst,
		 const env_light_desc&								 desc) noexcept;

	void
	save(handle _) noexcept;
}	 // namespace age::asset::env_light

namespace age::asset::model
{
	void
	full_unload(handle, auto& renderer) noexcept;

	void
	load(handle, auto& renderer) noexcept;

	handle
	load(std::string_view model_name, auto& renderer) noexcept;

	handle
	load_common_from_path(const age::array<char, config::max_asset_path_len>& full_path, auto& renderer) noexcept;

	bool
	renderable(handle h_model) noexcept;

	void
	update_mesh(handle h_model, handle h_mesh) noexcept;

	void
	update_material(handle h_model, uint32 idx, handle h_material) noexcept;

	void
	build(std::string_view model_path, const model_desc&) noexcept;

	void
	save(handle _) noexcept;
}	 // namespace age::asset::model

namespace age::asset::detail
{
	template <e::kind e_kind>
	handle
	load_common_from_path(const age::array<char, config::max_asset_path_len>& full_path) noexcept;

	template <e::kind>
	handle
	load_common(std::string_view asset_name) noexcept;
}	 // namespace age::asset::detail

namespace age::asset
{
	// macro magic
	AGE_DEFINE_ASSET_KIND(font, mesh_baked, material, texture, env_light, model);
}	 // namespace age::asset

namespace age::asset
{
	FORCE_INLINE constexpr bool
	is_fixable_by_normalize(e::asset_path_error_kind e) noexcept
	{
		return to_idx(e) > to_idx(e::asset_path_error_kind::fixable_by_normalize_begin);
	}

	template <e::kind e_kind>
	age::array<char, config::max_asset_path_len>
	get_path_safe(handle h_asset) noexcept
	{
		if (runtime::is_handle_invalid(h_asset))
		{
			return age::array<char, config::max_asset_path_len>{};
		}
		else
		{
			return h_asset.get_path<e_kind>();
		}
	}

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
			.asset_version		 = asset::get_asset_version<e_asset_kind>(),
		};
	}

	template <e::kind e_kind>
	void
	add_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e_kind>();
		AGE_DEBUG_LOG("asset add_ref {}, {}", to_string(e_kind), h.get_path<e_kind>());
		AGE_ASSERT(entry.ref_counter < std::numeric_limits<BARE_OF(entry.ref_counter)>::max());
		++entry.ref_counter;
	}

	template <e::kind e_kind>
	void
	remove_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e_kind>();
		AGE_DEBUG_LOG("asset remove_ref {}, {}", to_string(e_kind), h.get_path<e_kind>());
		AGE_ASSERT(entry.ref_counter > 0);
		--entry.ref_counter;
	}
}	 // namespace age::asset