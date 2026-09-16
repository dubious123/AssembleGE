#pragma once

// flow
//
//   .gltf file
//      |
//      v  external::cgltf::load_gltf          (C POD, faithful)
//   detail::gltf_data
//      |
//      v  external::cgltf::load               (type translation only)
//   gltf_parse_data                           (age types, faithful, not yet asset shaped)
//      |
//      v  importer::parse_gltf                (judge: errors, warnings, meshopt decode, spec-gloss)
//   gltf_parse_data + parse_error_vec / parse_warning_vec
//      |
//      v  importer::generate_gltf_import_data (asset shaped, dest paths, drop unconsumed)
//   gltf_import_data
//      |
//      v  importer::resolve_import            (re-run after every option / selection change, imgui)
//   gltf_import_data + import_error_vec / import_warning_vec, will_overwrite
//      |
//      v  importer::commit_import             (bake, write files, spawn entities)
//   gltf_commit_result
//
//
// legend
//   +-- x_vec        age::vector, owned
//   ---> y           uint32 index into another vec, invalid_id when absent
//   [asset kind]     what commit writes
//
//
// gltf_parse_data
// |
// +-- src_full_path
// +-- default_scene_idx ------------------------> scene
// |
// +-- mesh_parse_data_vec
// |     +-- gltf_mesh_parse_data
// |           +-- name
// |           +-- morph_weight_vec                          (not consumed)
// |           +-- submesh_vec
// |                 +-- gltf_submesh_parse_data
// |                       +-- position_vec normal_vec tangent_vec uv0_vec index_vec
// |                       +-- joints_vec weights_vec           (not consumed)
// |                       +-- morph_target_vec                 (not consumed)
// |                       |     +-- gltf_morph_target_data { position / normal / tangent delta_vec }
// |                       +-- material_idx ------------------> material
// |                       +-- mode, has_draco_compression, has_meshopt_compression
// |
// +-- texture_parse_data_vec
// |     +-- texture_parse_data { name, src_full_path | embedded, sampler_kind }
// |
// +-- material_parse_data_vec
// |     +-- material_parse_data
// |           +-- name
// |           +-- factors, normal_scale, occlusion_strength, alpha_cutoff
// |           +-- shading_model, double_sided
// |           +-- base_color / metallic_roughness / normal / occlusion / emissive
// |                 +-- material_texture_slot_parse_data { texture_idx ---> texture, texcoord }
// |
// +-- node_data_vec
// |     +-- gltf_node_data
// |           +-- name
// |           +-- translation, rotation, scale
// |           +-- parent_idx ------------------------------> node
// |           +-- mesh_idx --------------------------------> mesh
// |           +-- light_idx -------------------------------> light
// |           +-- camera_idx ------------------------------> camera
// |           +-- spawn
// |
// +-- scene_data_vec
// |     +-- gltf_scene_data { name, root_node_idx_vec ---> node, spawn }
// |
// +-- light_data_vec      +-- gltf_light_data  { kind, color, intensity, range, cone }
// +-- camera_data_vec     +-- gltf_camera_data { is_perspective, yfov_or_ymag, aspect, z_near, z_far }
// +-- skin_data_vec       +-- gltf_skin_data   { joint_node_idx_vec ---> node, inverse_bind_matrix_vec, skeleton_root_idx }   (not consumed)
// +-- animation_data_vec  +-- gltf_animation_data { name, channel_vec { target_node_idx ---> node, path, interpolation, time_vec, value_vec } }   (not consumed)
// |
// +-- parse_warning_vec   +-- gltf_parse_warning { kind, idx }
// +-- parse_error_vec     +-- gltf_parse_error   { kind, idx }
//
//
// gltf_import_data
// |
// +-- src_full_path
// +-- import_option { spawn_entity, generate_tangent, overwrite }
// |
// +-- mesh_import_data_vec                                   [mesh_baked]
// |     +-- mesh_baked_import_data
// |           +-- dest { path, h_created, will_overwrite, selected }
// |           +-- mesh : mesh_baked_parse_data { name, submesh_vec { position / normal / tangent / uv0 / index } }
// |           +-- bake_desc
// |
// +-- texture_import_data_vec                                [texture]
// |     +-- texture_import_data { dest, texture : texture_parse_data, format, generate_mip }
// |
// +-- material_import_data_vec                               [material]
// |     +-- material_import_data { dest, material : material_parse_data }
// |                                          slot.texture_idx ---> texture_import_data_vec, h_created at commit
// |
// +-- model_import_data_vec                                  [model], one per mesh
// |     +-- model_import_data { dest, mesh_idx ---> mesh_import_data_vec, material_idx_vec ---> material_import_data_vec }
// |
// +-- node_data_vec        (copied from parse, spawn = entity)
// +-- scene_data_vec       (copied from parse)
// +-- light_data_vec       (copied from parse)
// +-- camera_data_vec      (copied from parse)
// |
// +-- parse_warning_vec, parse_error_vec                     (carried over)
// +-- import_warning_vec   +-- gltf_import_warning { kind, idx }
// +-- import_error_vec     +-- gltf_import_error   { kind, idx }
//
//
// gltf_commit_result { created_count, overwritten_count, skipped_count, spawned_entity_count, success }
//
//
// reference flow
//
//   scene --> node --> mesh --> material --> texture
//                 +-> light
//                 +-> camera
//
//   model --> mesh, model --> material (one per submesh)
//   node.mesh_idx resolves to model_import_data_vec[mesh_idx] at spawn
//
//
// what is dropped between parse and import
//
//   parse                      import
//   -----                      ------
//   morph_target_vec           -                  warning: morph_dropped
//   joints_vec / weights_vec   -                  warning: skin_dropped
//   skin_data_vec              -                  warning: skin_dropped
//   animation_data_vec         -                  warning: animation_dropped
//   morph_weight_vec           -
//   uv1+                       -   (already dropped at parse, warning: uv1_dropped)
//   material variants          -   (already dropped at parse)


// texture
namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM(texture_file_kind, uint8, png, jpeg, webp, ktx2, unknown);


	AGE_DEFINE_ENUM_FLAGS(gltf_texture_parse_error_flags, uint8,
						  (none, 0u),
						  (image_file_not_found, (1u << 0u)),
						  (image_file_unreadable, (1u << 1u)));

	AGE_DEFINE_ENUM_FLAGS(gltf_texture_parse_warning_flags, uint8,
						  (none, (0u)),
						  (texture_kind_unknown, (1u << 0u)),	 // mime absent or contradicts the bytes, sniff decided
						  (fallback_image_used, (1u << 1u)));	 // no core source, webp / basisu image taken

	AGE_DEFINE_ENUM_FLAGS(texture_import_error_flags, uint8,
						  (none, 0u),
						  (image_file_not_found, (1u << 0u)),
						  (image_file_unreadable, (1u << 1u)),

						  (name_collision, (1u << 2u)),
						  (name_too_long, (1u << 3u)));

	AGE_DEFINE_ENUM_FLAGS(texture_import_warning_flags, uint16,
						  (none, (0u)),
						  (texture_kind_unknown, (1u << 0u)),
						  (fallback_image_used, (1u << 1u)),
						  (spec_gloss_not_converted, (1u << 2u)),
						  (source_empty, (1u << 3u)),

						  (alpha_dropped_by_format, (1u << 4u)),				   // referenced with alpha but the chosen format has no alpha channel
						  (alpha_missing_in_source, (1u << 5u)),				   // referenced with alpha but the source image has no alpha, samples as 1
						  (referenced_as_srgb_and_linear, (1u << 6u)),			   // base_color/emissive/ui_sprite and normal/mr/occlusion/... from different materials, srgb format chosen
						  (referenced_with_different_alpha_cutoff, (1u << 7u)),	   // mask materials disagree, first material's cutoff used
						  (file_already_exists, (1u << 8u)));

	AGE_DEFINE_ENUM_FLAGS(texture_usage_flags, uint16,
						  (none, (0u)),
						  (base_color, (1u << 0u)),
						  (metallic_roughness, (1u << 1u)),
						  (normal, (1u << 2u)),
						  (occlusion, (1u << 3u)),
						  (emissive, (1u << 4u)),
						  (alpha_mask, (1u << 5u)),		// need alpha_channel, alpha_cutoff
						  (alpha_blend, (1u << 6u)),	// need alpha_channle
						  (ibl, (1u << 7u)),
						  (font_msdf, (1u << 8u)),
						  (omm_mask, (1u << 9u)),
						  (ui_sprite, (1u << 10u)),		// srgb, no mips, clamp
						  (lut, (1u << 11u)));			// linear data, no mips, no compression

}	 // namespace age::asset::importer::e

namespace age::asset::importer
{
	struct gltf_texture_parse_data
	{
		importer::e::gltf_texture_parse_warning_flags warning_flags;
		importer::e::gltf_texture_parse_error_flags	  error_flags;

		std::string				 name;		   // image.name, empty is common
		dynamic_array<std::byte> bytes;
		uint32					 width;		   // 0 when file_kind is unknown
		uint32					 height;
		e::texture_file_kind	 file_kind;
		bool					 is_spec_gloss;
		bool					 has_alpha;	   // alpha channel or png tRNS, always false for jpeg
	};

	struct texture_import_data
	{
		// source, raw file bytes (png / jpg / ...). one for 2d, six for cube, n for array
		age::vector<dynamic_array<std::byte>>									source_bytes_vec;
		age::vector<std::pair<uint32 /*material_idx*/, e::texture_usage_flags>> material_reference_vec;
		age::vector<std::pair<uint32 /*model_idx   */, uint32 /*submesh_idx*/>> model_submesh_reference_vec;
		e::texture_file_kind													file_kind;	  // same for every entry
		bool																	is_spec_gloss;

		bool							enabled;
		e::texture_import_warning_flags warning_flags;
		e::texture_import_error_flags	error_flags;
		e::texture_usage_flags			usage_flags;
		bool							has_alpha;
		uint8							_;
		uint32							source_width;
		uint32							source_height;

		age::array<char, config::max_asset_display_name_len> name;			 // never empty
		texture_bake_option									 bake_option;	 // defaults from usage, user overrides on top
	};
}	 // namespace age::asset::importer

namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM_FLAGS(gltf_material_parse_warning_flags, uint8,
						  (none, (0u)),
						  (spec_gloss_converted, (1u << 0u)),
						  (texture_transform_dropped, (1u << 1u)),
						  (texcoord_not_0_dropped, (1u << 2u)),	   // slot cleared
						  (extension_dropped, (1u << 3u)));		   // clearcoat, sheen, ior, ...

	AGE_DEFINE_ENUM_FLAGS(material_import_warning_flags, uint8,
						  (none, (0u)),
						  (spec_gloss_converted, (1u << 0u)),
						  (texture_transform_dropped, (1u << 1u)),
						  (texcoord_not_0_dropped, (1u << 2u)),	   // slot cleared
						  (extension_dropped, (1u << 3u)),		   // clearcoat, sheen, ior, ...

						  (file_already_exists, (1u << 4u)));

	AGE_DEFINE_ENUM_FLAGS(material_import_error_flags, uint8,
						  (none, (0u)),

						  (texture_disabled, (1u << 0u)),
						  (name_collision, (1u << 1u)),
						  (name_too_long, (1u << 2u)));
}	 // namespace age::asset::importer::e

// materials
namespace age::asset::importer
{
	struct gltf_material_parse_data
	{
		importer::e::gltf_material_parse_warning_flags warning_flags;

		std::string name;

		float4 base_color_factor;
		float  metallic_factor;
		float  roughness_factor;
		float3 emissive_factor;
		float  normal_scale;
		float  occlusion_strength;
		float  alpha_cutoff;

		graphics::e::material_shading_model_kind shading_model;
		bool									 double_sided;

		graphics::e::sampler_kind base_color_sampler_kind;
		graphics::e::sampler_kind metallic_roughness_sampler_kind;
		graphics::e::sampler_kind normal_sampler_kind;
		graphics::e::sampler_kind occlusion_sampler_kind;
		graphics::e::sampler_kind emissive_sampler_kind;

		uint32 base_color_texture_idx		  = age::get_invalid_id<uint32>();	  // -> texture_parse_data_vec
		uint32 metallic_roughness_texture_idx = age::get_invalid_id<uint32>();
		uint32 normal_texture_idx			  = age::get_invalid_id<uint32>();
		uint32 occlusion_texture_idx		  = age::get_invalid_id<uint32>();
		uint32 emissive_texture_idx			  = age::get_invalid_id<uint32>();
	};

	struct material_import_data
	{
		importer::e::material_import_warning_flags warning_flags;
		importer::e::material_import_error_flags   error_flags;

		array<char, config::max_asset_display_name_len> name;

		float4 base_color_factor;
		float  metallic_factor;
		float  roughness_factor;
		float3 emissive_factor;
		float  normal_scale;
		float  occlusion_strength;
		float  alpha_cutoff;

		graphics::e::material_shading_model_kind shading_model;
		bool									 double_sided;

		graphics::e::sampler_kind base_color_sampler_kind;
		graphics::e::sampler_kind metallic_roughness_sampler_kind;
		graphics::e::sampler_kind normal_sampler_kind;
		graphics::e::sampler_kind occlusion_sampler_kind;
		graphics::e::sampler_kind emissive_sampler_kind;
		bool					  enabled;

		uint32 base_color_texture_idx		  = age::get_invalid_id<uint32>();	  // -> texture_import_data_vec
		uint32 metallic_roughness_texture_idx = age::get_invalid_id<uint32>();
		uint32 normal_texture_idx			  = age::get_invalid_id<uint32>();
		uint32 occlusion_texture_idx		  = age::get_invalid_id<uint32>();
		uint32 emissive_texture_idx			  = age::get_invalid_id<uint32>();

		age::vector<std::pair<uint32 /*model_idx   */, uint32 /*nth_material*/>> model_reference_vec;
	};


}	 // namespace age::asset::importer

// skeleton
namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM_FLAGS(gltf_skeleton_parse_error_flags, uint8,
						  (none, 0u),
						  (duplicated_joint_name, (1u << 0u)));	   // binding is by name, cannot resolve

	AGE_DEFINE_ENUM_FLAGS(gltf_skeleton_parse_warning_flags, uint8,
						  (none, (0u)),
						  (non_joint_node_in_tree, (1u << 0u)),	   // folded into the child joint's local
						  (joint_decompose_failed, (1u << 1u)),
						  (empty_joint_name, (1u << 2u)));

	AGE_DEFINE_ENUM_FLAGS(skeleton_import_error_flags, uint8,
						  (none, 0u),

						  (duplicated_joint_name, (1u << 0u)),
						  (name_collision, (1u << 1u)),
						  (name_too_long, (1u << 2u)),
						  (empty_joint_name, (1u << 3u)));

	AGE_DEFINE_ENUM_FLAGS(skeleton_import_warning_flags, uint8,
						  (none, (0u)),
						  (non_joint_node_in_tree, (1u << 0u)),	   // folded into the child joint's local
						  (joint_decompose_failed, (1u << 1u)),
						  (joint_name_generated, (1u << 2u)),	   // parse joint name is empty, default generated

						  (file_already_exists, (1u << 3u)));
}	 // namespace age::asset::importer::e

namespace age::asset::importer
{
	struct gltf_skeleton_joint_parse_data
	{
		uint32 parent_idx;	  // invalid == root, always < own index, skeleton_joint_idx
		float3 translation;
		float4 rotation;
		float3 scale;
	};

	struct gltf_skeleton_parse_data
	{
		importer::e::gltf_skeleton_parse_warning_flags warning_flags;
		importer::e::gltf_skeleton_parse_error_flags   error_flags;

		std::string									  name;
		dynamic_array<std::string>					  joint_name_vec;
		dynamic_array<gltf_skeleton_joint_parse_data> joint_vec;
	};

	struct skeleton_joint_import_data
	{
		uint32 parent_idx;	  // invalid == root, always < own index, skeleton_joint_idx
		uint32 subtree_count;
		float3 translation;
		float4 rotation;
		float3 scale;
	};

	struct skeleton_import_data
	{
		importer::e::skeleton_import_warning_flags warning_flags;
		importer::e::skeleton_import_error_flags   error_flags;
		bool									   enabled;
		uint8									   _;

		array<char, config::max_asset_display_name_len> name;
		vector<array<char, config::max_joint_name_len>> joint_name_vec;
		vector<skeleton_joint_import_data>				joint_vec;
	};
}	 // namespace age::asset::importer

// mesh_baked
namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM_FLAGS(gltf_mesh_parse_warning_flags, uint16,
						  (none, 0u),
						  (uv1_dropped, (1u << 0u)),
						  (color_dropped, (1u << 1u)),
						  (custom_attribute_dropped, (1u << 2u)),
						  (joints_over_4_merged, (1u << 3u)),				// JOINTS_1+ folded into top 4
						  (blend_shape_uv_dropped, (1u << 4u)),				// uv / color / custom deltas
						  (vertex_skin_zero_weight, (1u << 5u)),			// unweighted vertices stay rigid
						  (skin_without_skeleton, (1u << 6u)),				// JOINTS but no node binds a skin, skin dropped
						  (unskinned_submesh_rigid, (1u << 7u)),			// some submeshes lack JOINTS, zero weights
						  (empty_joint_name, (1u << 8u)),
						  (skin_dropped_by_multiple_skins, (1u << 9u)));	// 1mesh, n skins (different skin byte data, skin idx may differ) -> other skin is dropped

	AGE_DEFINE_ENUM_FLAGS(gltf_mesh_parse_error_flags, uint8,
						  (none, 0u),
						  (draco_compressed, (1u << 0u)),					// no decoder
						  (meshopt_compressed, (1u << 1u)),					// not decoded yet
						  (unsupported_topology, (1u << 2u)),
						  (position_missing, (1u << 3u)),
						  (vertex_idx_out_of_range, (1u << 4u)),			//  index_buffer[i] >= vertex_buffer.size()
						  (blend_shape_count_mismatch, (1u << 5u)),
						  (joint_idx_out_of_range, (1u << 6u)));

	AGE_DEFINE_ENUM_FLAGS(mesh_import_warning_flags, uint16,
						  (none, 0u),
						  (uv1_dropped, (1u << 0u)),
						  (color_dropped, (1u << 1u)),
						  (custom_attribute_dropped, (1u << 2u)),
						  (joints_over_4_merged, (1u << 3u)),		// JOINTS_1+ folded into top 4
						  (blend_shape_uv_dropped, (1u << 4u)),		// uv / color / custom deltas
						  (vertex_skin_zero_weight, (1u << 5u)),	// unweighted vertices stay rigid
						  (skin_without_skeleton, (1u << 6u)),		// JOINTS but no node binds a skin, skin dropped
						  (unskinned_submesh_rigid, (1u << 7u)),	// some submeshes lack JOINTS, zero weights
						  (joint_name_generated, (1u << 8u)),
						  (gltf_skin_dropped_by_multiple_skins, (1u << 9u)),

						  (file_already_exists, (1u << 10u)),
						  (duplicated_blend_shape_name, (1u << 11u)),
						  (empty_blend_shape_name, (1u << 12u)));

	AGE_DEFINE_ENUM_FLAGS(mesh_import_error_flags, uint16,
						  (none, 0u),
						  (draco_compressed, (1u << 0u)),			// no decoder
						  (meshopt_compressed, (1u << 1u)),			// not decoded yet
						  (unsupported_topology, (1u << 2u)),
						  (position_missing, (1u << 3u)),
						  (vertex_idx_out_of_range, (1u << 4u)),	//  index_buffer[i] >= vertex_buffer.size()
						  (blend_shape_count_mismatch, (1u << 5u)),
						  (joint_idx_out_of_range, (1u << 6u)),

						  (name_collision, (1u << 7u)),
						  (name_too_long, (1u << 8u)),
						  (duplicated_joint_name, (1u << 9u)),
						  (empty_joint_name, (1u << 10u)));
}	 // namespace age::asset::importer::e

namespace age::asset::importer
{
	struct vertex_skin_data
	{
		uint16_4 joint_idx = uint16_4::zero();
		float4	 weight	   = float4::zero();	// sum == 1.f or 0.f
												// sum == 0.f : zero_weight_vertex_rigid or unskinned_submesh_rigid
	};

	struct blend_shape_parse_data
	{
		dynamic_array<float3> position_delta_buffer;
		dynamic_array<float3> normal_delta_buffer;	   // empty = none
		dynamic_array<float3> tangent_delta_buffer;	   // empty = none
	};

	struct gltf_submesh_parse_data
	{
		dynamic_array<asset::vertex_fat> vertex_buffer;
		dynamic_array<uint32>			 index_buffer;

		dynamic_array<vertex_skin_data>		  vertex_skin_buffer;	 // empty = none
		dynamic_array<blend_shape_parse_data> blend_shape_vec;		 // size == mesh.blend_shape_name_vec.size()

		bool has_normal;
		bool has_tangent;
		bool has_uv;

		graphics::e::mesh_raster_mode_kind		  raster_mode		 = graphics::e::mesh_raster_mode_kind::opaque;
		graphics::e::mesh_rt_alpha_test_mode_kind rt_alpha_test_mode = graphics::e::mesh_rt_alpha_test_mode_kind::blend;
		graphics::e::mesh_rt_bake_mode_kind		  rt_bake_mode		 = graphics::e::mesh_rt_bake_mode_kind::opaque;
	};

	struct gltf_mesh_baked_parse_data
	{
		importer::e::gltf_mesh_parse_warning_flags warning_flags;
		importer::e::gltf_mesh_parse_error_flags   error_flags;
		uint8									   _;

		std::string							   name;
		dynamic_array<gltf_submesh_parse_data> submesh_vec;

		dynamic_array<std::string> joint_name_vec;			// empty == no skeleton
		dynamic_array<float4x4>	   mesh_to_joint_vec;		// empty == no skeleton, joint_name_vec.size() == mesh_to_joint_vec.size()
		dynamic_array<std::string> blend_shape_name_vec;	// empty == no blend shape
		dynamic_array<float>	   blend_shape_weight_vec;
	};

	struct submesh_import_data
	{
		age::vector<uint32>			   index_buffer;
		age::vector<asset::vertex_fat> vertex_buffer;

		age::vector<vertex_skin_data>		vertex_skin_buffer;	   // empty = none
		age::vector<blend_shape_parse_data> blend_shape_vec;	   // size == mesh.blend_shape_name_vec.size()

		bool has_normal;
		bool has_tangent;
		bool has_uv;

		graphics::e::mesh_raster_mode_kind		  raster_mode		 = graphics::e::mesh_raster_mode_kind::opaque;
		graphics::e::mesh_rt_alpha_test_mode_kind rt_alpha_test_mode = graphics::e::mesh_rt_alpha_test_mode_kind::blend;
		graphics::e::mesh_rt_bake_mode_kind		  rt_bake_mode		 = graphics::e::mesh_rt_bake_mode_kind::opaque;

		bool
		need_alpha_channel() const
		{
			return raster_mode != graphics::e::mesh_raster_mode_kind::opaque
				or rt_alpha_test_mode != graphics::e::mesh_rt_alpha_test_mode_kind::opaque;
		}
	};

	struct mesh_baked_import_data
	{
		importer::e::mesh_import_warning_flags warning_flags;
		importer::e::mesh_import_error_flags   error_flags;
		bool								   enabled;
		uint8_3								   _;

		age::array<char, config::max_asset_display_name_len> name;
		age::vector<submesh_import_data>					 submesh_vec;

		age::vector<array<char, config::max_joint_name_len>>	   joint_name_vec;			// empty == no skeleton
		age::vector<float4x4>									   mesh_to_joint_vec;		// empty == no skeleton, joint_name_vec.size() == mesh_to_joint_vec.size()
		age::vector<array<char, config::max_blend_shape_name_len>> blend_shape_name_vec;	// empty == no blend shape
		age::vector<float>										   blend_shape_weight_vec;

		age::vector<uint32> model_reference_vec;
	};
}	 // namespace age::asset::importer

// model
namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM_FLAGS(gltf_model_parse_warning_flags, uint8,
						  (none, 0u),
						  (material_variants_dropped, (1u << 0u)));	   // KHR_materials_variants, default mapping kept

	AGE_DEFINE_ENUM_FLAGS(gltf_model_parse_error_flags, uint8,
						  (none, 0u));


	AGE_DEFINE_ENUM_FLAGS(model_import_warning_flags, uint8,
						  (none, 0u),
						  (material_variants_dropped, (1u << 0u)),	  // KHR_materials_variants, default mapping kept

						  (file_already_exists, (1u << 1u)),
						  (material_without_submesh, (1u << 2u)));

	AGE_DEFINE_ENUM_FLAGS(model_import_error_flags, uint8,
						  (none, 0u),

						  (name_collision, (1u << 0u)),
						  (name_too_long, (1u << 1u)),
						  (mesh_disabled, (1u << 2u)),
						  (material_disabled, (1u << 3u)),
						  (skeleton_disabled, (1u << 4u)),
						  (submesh_without_material, (1u << 5u)));
}	 // namespace age::asset::importer::e

namespace age::asset::importer
{
	struct gltf_model_parse_data
	{
		importer::e::gltf_model_parse_warning_flags warning_flags;
		importer::e::gltf_model_parse_error_flags	error_flags;

		uint32				  mesh_idx	   = age::get_invalid_id<uint32>();	   // -> mesh_parse_data_vec
		uint32				  skeleton_idx = age::get_invalid_id<uint32>();	   // -> skeleton_parse_data_vec, valid iff mesh has skin
		dynamic_array<uint32> submesh_gltf_material_idx_vec;				   // -> gltf_material_parse_data_vec, size == mesh.submesh_vec.size()
	};

	struct model_import_data
	{
		importer::e::model_import_warning_flags warning_flags;
		importer::e::model_import_error_flags	error_flags;
		bool									enabled;
		uint8									_;

		age::array<char, config::max_asset_display_name_len> name;
		uint32												 mesh_idx = age::get_invalid_id<uint32>();
		age::vector<uint32>									 submesh_material_idx_vec;
	};
};	  // namespace age::asset::importer

// scene, entity
namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM_FLAGS(gltf_entity_parse_warning_flags, uint8,
						  (none, (0u)),
						  (decompose_trs_failed, (1u << 0u)),							// node matrix has shear or zero-length axis, rotation set to identity
						  (instancing_dropped, (1u << 1u)),								// EXT_mesh_gpu_instancing, model_idx set to invalid
						  (joint_attach_dropped_by_skin_mismatch, (1u << 2u)),			// joint_attach skeleton owner != skinned_model skeleton owner, skinned_model wins
						  (skin_on_static_mesh, (1u << 3u)));

	AGE_DEFINE_ENUM_FLAGS(entity_import_warning_flags, uint16,
						  (none, (0u)),
						  (decompose_trs_failed, (1u << 0u)),							// node matrix has shear or zero-length axis, rotation set to identity
						  (instancing_dropped, (1u << 1u)),								// EXT_mesh_gpu_instancing, model_idx set to invalid
						  (joint_attach_dropped_by_skin_mismatch, (1u << 2u)),			// joint_attach skeleton owner != skinned_model skeleton owner, skinned_model wins
						  (skeleton_owner_scale_zero, (1u << 3u)),
						  (forced_root_by_skinned_mesh_cycle, (1u << 4u)),				// cycle by skinned mesh reparent, goto import_gltf_scenes loop2 pre-pass for more details
						  (joint_attach_dropped_by_skinned_mesh_cycle, (1u << 5u)),		// cycle by skinned mesh reparent, joint_attach dropped, goto import_gltf_scenes loop2 pre-pass for more details
						  (skinned_model_dropped_by_skinned_mesh_cycle, (1u << 6u)),	// cycle by skinned mesh reparent, is_skinned and model dropped, goto import_gltf_scenes loop2 pre-pass for more details

						  (skin_on_static_mesh, (1u << 7u)),
						  (skinned_but_model_missing, (1u << 9u)),
						  (skinned_but_mesh_missing, (1u << 10u)),
						  (skinned_and_skeleton_disabled, (1u << 11u)));

	AGE_DEFINE_ENUM_FLAGS(entity_import_error_flags, uint8,
						  (none, (0u)),

						  (joint_attach_skeleton_missing, (1u << 0u)),
						  (joint_attach_skeleton_mismatch, (1u << 1u)),
						  (skinned_model_skeleton_mismatch, (1u << 2u)),
						  (owner_skeleton_disabled, (1u << 3u)),
						  (referenced_skeleton_is_disabled, (1u << 4u)),
						  (skinned_but_skeleton_missing, (1u << 5u)),
						  (scale_is_zero, (1u << 6u)));					   // scale has zero, fix by ui


	AGE_DEFINE_ENUM_FLAGS(scene_import_warning_flags, uint8,
						  (none, (0u)),
						  (entity_hierarchy_cycle_dropped, (1u << 0u)),	   // cycle detected and will be dropped. goto import_gltf_scenes loop 3 for more details

						  (file_already_exists, (1u << 1u)));

	AGE_DEFINE_ENUM_FLAGS(scene_import_error_flags, uint8,
						  (none, (0u)),

						  (name_collision, (1u << 0u)),
						  (name_too_long, (1u << 1u)),
						  (target_storage_missing_component, (1u << 2u)));
}	 // namespace age::asset::importer::e

namespace age::asset::importer
{
	// components
	struct gltf_light_parse_data
	{
		graphics::e::light_kind kind;				 // directional / point / spot
		float3					color;
		float					intensity;
		float					range;				 // point / spot, 0 = infinite
		float					inner_cone_angle;	 // spot
		float					outer_cone_angle;	 // spot
	};

	struct gltf_camera_parse_data
	{
		bool  is_perspective;
		float yfov_or_ymag;
		float aspect;	 // 0 = unspecified
		float z_near;
		float z_far;	 // 0 = infinite
	};

	struct gltf_entity_parse_data
	{
		importer::e::gltf_entity_parse_warning_flags warning_flags;

		std::string name;
		float3		translation;
		float4		rotation;
		float3		scale;
		uint32		parent_idx = age::get_invalid_idx<uint32>();	// -> entity_parse_data_vec, invalid == root, always < own index
		uint32		model_idx  = age::get_invalid_idx<uint32>();	// -> model_parse_data_vec
		uint32		light_idx  = age::get_invalid_idx<uint32>();
		uint32		camera_idx = age::get_invalid_idx<uint32>();
		// this entity is skinned_model or joint_attach or both
		// if (model_idx is_invalid or model.mesh_idx is_invalid) and skeleton_joint_idx is_invalid => skin_without_mesh warning
		// if model_idx is_not_invalid => skinned_model
		// skeleton owner entity's skeleton_idx must not invalid_idx
		uint32 skeleton_owner_entity_idx = age::get_invalid_idx<uint32>();

		// this entity is the [skeleton_joint_idx]_th joint of the skeleton owner
		// skeleton_owner_entity_idx is always valid
		// joint_attach
		uint32 skeleton_joint_idx = age::get_invalid_idx<uint32>();

		// model_idx + is_skinned => skinned_model component
		// model_idx              => model component (static)
		bool	is_skinned;
		uint8_3 _;

		// this entity is the owner of a skeleton
		uint32				 skeleton_idx = age::get_invalid_idx<uint32>();
		dynamic_array<float> blend_shape_weight_override_arr;	 // empty = mesh default
	};

	struct gltf_scene_parse_data
	{
		std::string name;
		uint32		entity_begin;	 // entity_parse_data_vec[begin, begin + count)
		uint32		entity_count;
	};

	struct entity_import_data
	{
		importer::e::entity_import_warning_flags warning_flags;
		importer::e::entity_import_error_flags	 error_flags;

		bool	enabled;
		bool	is_skinned;
		uint8_3 _;

		age::array<char, config::max_entity_name_len> name;

		uint32 light_idx;
		uint32 camera_idx;
		uint32 model_idx;
		uint32 parent_idx;
		uint32 skeleton_idx;	// this entity is the owner of a skeleton
		uint32 joint_attach_idx;
		uint32 blend_shape_weight_override_arr_idx;
		// local trs
		float3 translation;
		float4 rotation;
		float3 scale;

		age::vector<uint32> child_idx_vec;
	};

	struct scene_import_data
	{
		struct light_data
		{
			bool					enabled;
			graphics::e::light_kind kind;
			bool					cast_shadow;
			uint8					_;

			float  range;
			float3 direction;
			float  intensity;
			float3 color;
			float  cos_inner = 0.5f;
			float  cos_outer = 0.4f;
		};

		struct camera_data
		{
			bool					 enabled;
			graphics::e::camera_kind kind;
			uint8_2					 _;

			float3 euler_deg = float3::zero();

			float near_z = 0.1f;
			float far_z	 = 1000.f;

			float fov_y		   = age::cvt_to_radian(75.f);
			float aspect_ratio = (16.f / 9.f);

			// orthographics
			float view_width  = 1.f;
			float view_height = 1.f;
		};

		struct joint_attach_data
		{
			bool										 enabled;
			age::array<char, config::max_joint_name_len> joint_name;
		};

		e::scene_import_error_flags	  error_flags;
		e::scene_import_warning_flags warning_flags;
		bool						  enabled;
		bool						  instantiate;

		bool has_hierarchy;
		bool has_model;
		bool has_skinned_model;
		bool has_skeleton;

		bool has_directional_light;
		bool has_point_light;
		bool has_spot_light;
		bool has_blend_shape_weight_override;

		age::array<char, config::max_scene_name_len> name;

		// for some reasons (e.g. cycle, entity dropped...), components may become an orphan,
		// not a big problem but <component>_data_vec.size() may differ from real component count
		age::vector<light_data>			light_data_vec;
		age::vector<joint_attach_data>	joint_attach_data_vec;
		age::vector<camera_data>		camera_data_vec;
		age::vector<age::vector<float>> blend_shape_weight_override_arr_vec;
		age::vector<entity_import_data> entity_vec;

		uint32 instantiate_target_scene_idx;
		uint32 instantiate_target_storage_idx;
	};
}	 // namespace age::asset::importer

namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM(gltf_parse_error_kind, uint8,
					none,
					file_not_found,		 // .gltf / .glb
					invalid_format,		 // not glTF, broken json, spec violation
					buffer_not_found,	 // .bin missing, io error
					internal);			 // out of memory

	AGE_DEFINE_ENUM_FLAGS(gltf_parse_warning_flags, uint8,
						  (none, (0u)),
						  (animation_dropped, (1u << 0u)),
						  (entity_not_in_scene_dropped, (1u << 1u)),
						  (duplicated_entity_in_scene_dropped, (1u << 2u)),
						  (skeleton_owner_node_not_in_scene, (1u << 3u)),	 // skeleton owner gltf node is in another scene. an empty owner entity is generated for this scene (skeleton_owner_entity_generated is also set)
						  (skeleton_owner_entity_generated, (1u << 4u)));

	AGE_DEFINE_ENUM_FLAGS(import_warning_flags, uint8,
						  (none, (0u)),
						  (animation_dropped, (1u << 0u)),
						  (entity_not_in_scene_dropped, (1u << 1u)),
						  (duplicated_entity_in_scene_dropped, (1u << 2u)),
						  (skeleton_owner_gltf_node_not_in_scene, (1u << 3u)),	  // skeleton owner gltf node is in another scene. an empty owner entity is generated for this scene (skeleton_owner_entity_generated is also set)
						  (skeleton_owner_entity_generated, (1u << 4u)));

	AGE_DEFINE_ENUM_FLAGS(import_error_flags, uint8,
						  (none, (0u)),
						  (file_not_found, (1u << 0u)),
						  (invalid_format, (1u << 1u)),
						  (buffer_not_found, (1u << 2u)),
						  (internal, (1u << 3u)));


}	 // namespace age::asset::importer::e

// import
namespace age::asset::importer
{
	struct gltf_parse_data
	{
		importer::e::gltf_parse_warning_flags warning_flags;
		importer::e::gltf_parse_error_kind	  error = importer::e::gltf_parse_error_kind::none;

		std::filesystem::path src_full_path;

		age::vector<gltf_texture_parse_data>	gltf_texture_parse_data_vec;
		age::vector<gltf_mesh_baked_parse_data> gltf_mesh_parse_data_vec;
		age::vector<gltf_skeleton_parse_data>	gltf_skeleton_parse_data_vec;
		age::vector<gltf_material_parse_data>	gltf_material_parse_data_vec;
		age::vector<gltf_model_parse_data>		gltf_model_parse_data_vec;
		age::vector<gltf_light_parse_data>		gltf_light_parse_data_vec;
		age::vector<gltf_camera_parse_data>		gltf_camera_parse_data_vec;
		age::vector<gltf_entity_parse_data>		gltf_entity_parse_data_vec;
		age::vector<gltf_scene_parse_data>		gltf_scene_parse_data_vec;
		uint32									default_scene_idx = age::get_invalid_id<uint32>();

		bool
		is_valid() const noexcept;	  // error == none && every texture / mesh error_flags == 0
	};

	struct import_data
	{
		importer::e::import_warning_flags warning_flags;
		importer::e::import_error_flags	  error_flags;

		bool	has_error;
		uint8_3 _;

		std::string											 src_full_path;
		age::array<char, config::max_asset_display_name_len> asset_name;

		age::array<char, config::max_asset_path_len> texture_dir;
		age::array<char, config::max_asset_path_len> material_dir;
		age::array<char, config::max_asset_path_len> mesh_dir;
		age::array<char, config::max_asset_path_len> skeleton_dir;
		age::array<char, config::max_asset_path_len> model_dir;
		age::array<char, config::max_asset_path_len> scene_dir;

		age::vector<texture_import_data>	texture_import_data_vec;
		age::vector<material_import_data>	material_import_data_vec;
		age::vector<mesh_baked_import_data> mesh_import_data_vec;
		age::vector<skeleton_import_data>	skeleton_import_data_vec;
		age::vector<model_import_data>		model_import_data_vec;
		age::vector<scene_import_data>		scene_import_data_vec;
	};


}	 // namespace age::asset::importer

namespace age::asset::importer::e
{
	AGE_DEFINE_ENUM(commit_error_kind, uint8,
					none,
					commit_temporary_dir_not_empty,
					commit_temporary_dir_create_failed,
					create_target_dir_failed,
					tex_temporary_file_create_failed,
					tex_bake_failed,
					temp_file_cleanup_failed,
					temp_dir_cleanup_failed,
					target_path_cannot_be_overwritten,
					backup_failed_and_rollback_failed,
					backup_failed,
					move_failed_and_rollback_failed,
					move_failed);
}

namespace age::asset::importer
{
	struct commit_result
	{
		e::commit_error_kind error;
		asset::e::kind		 asset_kind;
		uint32				 idx_0;
		uint32				 idx_1;
		std::string			 message;
	};
}	 // namespace age::asset::importer