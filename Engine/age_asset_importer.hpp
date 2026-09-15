#pragma once
#include "age.hpp"

namespace age::asset::importer
{
	gltf_parse_data
	parse_gltf(const std::filesystem::path& gltf_full_path) noexcept;

	// asset_root_dir : root relative
	void
	resolve_import(import_data&) noexcept;

	gltf_commit_result
	commit_import(import_data&) noexcept;
}	 // namespace age::asset::importer

namespace age::asset::importer
{
	// texture
	void
	fill_texture_default_import_data(texture_import_data& res, e::texture_usage_flags usage_flags) noexcept;
	texture_import_data
	generate_texture_import_data(gltf_texture_parse_data&& parse, e::texture_usage_flags usage, const std::string& name) noexcept;

	skeleton_import_data
	generate_skeleton_import_data(gltf_skeleton_parse_data&& parse, const std::string& name) noexcept;

	material_import_data
	generate_material_import_data(gltf_material_parse_data&& parse, const std::string& name) noexcept;

	mesh_baked_import_data
	generate_mesh_baked_import_data(gltf_mesh_baked_parse_data&& parse, const std::string& name) noexcept;

	model_import_data
	generate_model_import_data(gltf_model_parse_data&& parse, const std::string& name) noexcept;

	scene_import_data
	generate_scene_import_data(gltf_scene_parse_data&& parse, const std::string& name) noexcept;

	import_data
	generate_gltf_import_data(gltf_parse_data&& gltf_parse, std::string_view asset_name, const std::filesystem::path& target_dir) noexcept;
}	 // namespace age::asset::importer

// helpers
namespace age::asset::importer
{
	// [kind, extent, has_alpha]
	std::tuple<e::texture_file_kind, extent_2d<uint32>, bool>
	get_texture_source_info(std::span<const std::byte>, std::string_view path) noexcept;

	// [base_color, metallic, roughness]
	std::tuple<float4, float, float>
	cvt_spec_gloss_to_mr(const float4& diffuse, const float3& specular, float glossiness) noexcept;
}	 // namespace age::asset::importer