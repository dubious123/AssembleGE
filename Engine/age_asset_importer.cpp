#include "age_pch.hpp"
#include "age.hpp"

// texture
namespace age::asset::importer
{
	std::tuple<e::texture_file_kind, extent_2d<uint32>, bool>
	get_texture_source_info(std::span<const std::byte> bytes, std::string_view path) noexcept
	{
		using enum e::texture_file_kind;

		c_auto bytes_size	   = bytes.size();
		c_auto read_uint8	   = [&](std::size_t i) { return static_cast<uint32>(std::to_integer<uint8>(bytes[i])); };
		c_auto read_uint16_be  = [&](std::size_t i) { return read_uint8(i) << 8 | read_uint8(i + 1); };
		c_auto read_uint32_be  = [&](std::size_t i) { return read_uint8(i) << 24 | read_uint8(i + 1) << 16 | read_uint8(i + 2) << 8 | read_uint8(i + 3); };
		c_auto read_uint16_le  = [&](std::size_t i) { return read_uint8(i) | read_uint8(i + 1) << 8; };
		c_auto read_uint8_3_le = [&](std::size_t i) { return read_uint8(i) | read_uint8(i + 1) << 8 | read_uint8(i + 2) << 16; };
		c_auto read_uint32_le  = [&](std::size_t i) { return read_uint8_3_le(i) | read_uint8(i + 3) << 24; };
		c_auto matches_at	   = [&](std::size_t off, std::string_view s) { return bytes_size >= off + s.size()
																			   and std::string_view{ reinterpret_cast<const char*>(bytes.data()), bytes_size }.substr(off, s.size()) == s; };

		auto res_kind  = unknown;
		auto extent	   = extent_2d<uint32>{ 0, 0 };
		auto has_alpha = false;

		// png: IHDR is always first, tRNS may add transparency to color type 0 / 2 / 3
		if (matches_at(0, "\x89PNG\r\n\x1a\n"))
		{
			res_kind = png;
			if (bytes_size >= 29 and matches_at(12, "IHDR"))
			{
				extent.width  = read_uint32_be(16);
				extent.height = read_uint32_be(20);
				c_auto type	  = read_uint8(25);
				has_alpha	  = type == 4 or type == 6;
				for (std::size_t pos = 8; has_alpha is_false and pos + 8 <= bytes_size;)
				{
					c_auto len = static_cast<std::size_t>(read_uint32_be(pos));
					if (matches_at(pos + 4, "IDAT") or matches_at(pos + 4, "IEND"))
					{
						break;
					}
					if (matches_at(pos + 4, "tRNS"))
					{
						has_alpha = true;
					}
					if (len > bytes_size)
					{
						break;
					}
					pos += 12 + len;
				}
			}
		}

		// jpeg: walk segments to SOF0..SOF15 (excluding DHT / JPG / DAC), never has alpha
		else if (matches_at(0, "\xff\xd8\xff"))
		{
			res_kind = jpeg;
			for (std::size_t pos = 2; pos + 4 <= bytes_size;)
			{
				if (read_uint8(pos) != 0xff) { break; }

				c_auto marker = read_uint8(pos + 1);
				// fill byte
				if (marker == 0xff)
				{
					++pos;
					continue;
				}
				// standalone
				if (marker == 0xd8 or marker == 0x01 or (marker >= 0xd0 and marker <= 0xd7))
				{
					pos += 2;
					continue;
				}
				c_auto len = static_cast<std::size_t>(read_uint16_be(pos + 2));
				c_auto sof = marker >= 0xc0 and marker <= 0xcf and marker != 0xc4 and marker != 0xc8 and marker != 0xcc;
				if (sof)
				{
					if (pos + 9 <= bytes_size)
					{
						extent.height = read_uint16_be(pos + 5);
						extent.width  = read_uint16_be(pos + 7);
					}
					break;
				}
				// SOS: no SOF before scan
				if (marker == 0xda or len < 2)
				{
					break;
				}
				pos += 2 + len;
			}
		}

		// webp: simple lossy / lossless / extended headers differ
		else if (matches_at(0, "RIFF") and matches_at(8, "WEBP") and bytes_size >= 30)
		{
			res_kind = webp;
			if (matches_at(12, "VP8 "))
			{
				if (read_uint8(23) == 0x9d and read_uint8(24) == 0x01 and read_uint8(25) == 0x2a)
				{
					extent.width  = read_uint16_le(26) & 0x3fff;
					extent.height = read_uint16_le(28) & 0x3fff;
				}
			}
			else if (matches_at(12, "VP8L") and read_uint8(20) == 0x2f)
			{
				c_auto bits	  = read_uint32_le(21);
				extent.width  = (bits & 0x3fff) + 1;
				extent.height = ((bits >> 14) & 0x3fff) + 1;
				has_alpha	  = ((bits >> 28) & 1) != 0;
			}
			else if (matches_at(12, "VP8X"))
			{
				has_alpha	  = (read_uint8(20) & 0x10) != 0;
				extent.width  = read_uint8_3_le(24) + 1;
				extent.height = read_uint8_3_le(27) + 1;
			}
		}

		// ktx2: 80 byte header, alpha from vkFormat
		else if (matches_at(0, "\xabKTX 20\xbb\r\n\x1a\n"))
		{
			res_kind = ktx2;
			if (bytes_size >= 80)
			{
				c_auto func_in	 = [](uint32 vk_format, uint32 lo, uint32 hi) { return vk_format >= lo and vk_format <= hi; };
				c_auto vk_format = read_uint32_le(12);
				has_alpha		 = func_in(vk_format, 2, 3)			// R4G4B4A4 / B4G4R4A4 pack16
								or func_in(vk_format, 6, 8)			// R5G5B5A1 / B5G5R5A1 / A1R5G5B5 pack16
								or func_in(vk_format, 37, 43)		// R8G8B8A8 unorm .. srgb
								or func_in(vk_format, 44, 50)		// B8G8R8A8 unorm .. srgb
								or func_in(vk_format, 51, 57)		// A8B8G8R8 pack32
								or func_in(vk_format, 58, 69)		// A2R10G10B10 / A2B10G10R10 pack32
								or func_in(vk_format, 91, 97)		// R16G16B16A16 unorm .. sfloat
								or func_in(vk_format, 107, 109)		// R32G32B32A32 uint vk_format, / sint vk_format, / sfloat
								or func_in(vk_format, 119, 121)		// R64G64B64A64
								or func_in(vk_format, 133, 134)		// BC1 RGBA (131 / 132 are BC1 RGB, no alpha)
								or func_in(vk_format, 135, 138)		// BC2, BC3
								or func_in(vk_format, 145, 146)		// BC7
								or func_in(vk_format, 149, 152)		// ETC2 R8G8B8A1, R8G8B8A8
								or func_in(vk_format, 157, 184);	// ASTC 4x4 .. 12x12, always RGBA

				extent.width  = read_uint32_le(20);
				extent.height = read_uint32_le(24);
			}
		}
		else
		{
			// magic unknown: fall back to the extension, no dimensions
			c_auto ends_with = [&](std::string_view suffix) {
				return path.size() >= suffix.size()
				   and std::ranges::equal(path.substr(path.size() - suffix.size()), suffix, [](char a, char b) { return util::to_lower_ascii(a) == util::to_lower_ascii(b); });
			};

			res_kind = ends_with(".png")
						 ? png
					 : ends_with(".jpg") or ends_with(".jpeg")
						 ? jpeg
					 : ends_with(".webp")
						 ? webp
					 : ends_with(".ktx2")
						 ? ktx2
						 : unknown;
		}
		return { res_kind, extent, has_alpha };
	}

	e::texture_import_warning_flags
	get_import_flags(e::gltf_texture_parse_warning_flags gltf_warning_flags) noexcept
	{
		using src = e::gltf_texture_parse_warning_flags;
		using dst = e::texture_import_warning_flags;

		auto res = dst::none;
		if (e::has_all(gltf_warning_flags, src::texture_kind_unknown)) { res |= dst::texture_kind_unknown; }
		if (e::has_all(gltf_warning_flags, src::fallback_image_used)) { res |= dst::fallback_image_used; }
		return res;
	}

	e::texture_import_error_flags
	get_import_flags(e::gltf_texture_parse_error_flags gltf_error_flags) noexcept
	{
		using src = e::gltf_texture_parse_error_flags;
		using dst = e::texture_import_error_flags;

		auto res = dst::none;
		if (e::has_all(gltf_error_flags, src::image_file_not_found)) { res |= dst::image_file_not_found; }
		if (e::has_all(gltf_error_flags, src::image_file_unreadable)) { res |= dst::image_file_unreadable; }
		return res;
	}

	graphics::e::texture_format
	get_default_texture_format(e::texture_usage_flags usage_flags) noexcept
	{
		using enum e::texture_usage_flags;
		using enum graphics::e::texture_format;

		if (e::has_any(usage_flags, base_color | emissive | ui_sprite)) { return bc7_unorm_srgb; }
		if (e::has_all(usage_flags, normal)) { return bc5_unorm; }
		if (e::has_any(usage_flags, lut | font_msdf)) { return rgba8_unorm; }	 // data, no compression
		if (e::has_any(usage_flags, metallic_roughness | occlusion | omm_mask | ibl)) { return bc7_unorm; }
		return bc7_unorm;														 // metallic_roughness, occlusion, omm_mask, ibl, unreferenced
	}

	void
	fill_texture_default_import_data(texture_import_data& res) noexcept
	{
		using enum e::texture_usage_flags;

		auto usage_flags = e::texture_usage_flags::none;
		for (c_auto& [ _, usage ] : res.material_reference_vec)
		{
			usage_flags |= usage;
		}

		res.bake_option.format			= get_default_texture_format(usage_flags);
		res.bake_option.invert_y		= e::has_all(usage_flags, normal);
		res.bake_option.mip_count		= e::has_any(usage_flags, ui_sprite | lut) ? 1u : 0u;
		res.bake_option.alpha_threshold = -1.f;
		res.bake_option.keep_coverage	= -1.f;
	}

	texture_import_data
	generate_texture_import_data(gltf_texture_parse_data&& parse_data, const std::string& name) noexcept
	{
		using enum e::texture_usage_flags;
		auto res	= texture_import_data{};
		res.enabled = true;

		// gltf textures are always a single 2d image
		res.source_bytes_vec.emplace_back(std::move(parse_data.bytes));

		res.file_kind	  = parse_data.file_kind;
		res.is_spec_gloss = parse_data.is_spec_gloss;
		res.has_alpha	  = parse_data.has_alpha;
		res.source_width  = parse_data.width;
		res.source_height = parse_data.height;

		// plan
		res.warning_flags	 = get_import_flags(parse_data.warning_flags);
		res.error_flags		 = get_import_flags(parse_data.error_flags);
		res.name			 = util::to_fixed_str<config::max_asset_display_name_len>(name);
		res.bake_option.wrap = asset::e::wrap_mode_kind::wrap;

		return res;
	}
}	 // namespace age::asset::importer

// material
namespace age::asset::importer
{
	// based on Khronos "convert-between-workflows" 2017
	// three.pbrUtilities.js
	// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows/js/three.pbrUtilities.js
	std::tuple<float4, float, float>
	cvt_spec_gloss_to_mr(const float4& diffuse, const float3& specular, float glossiness) noexcept
	{
		constexpr auto dielectric = 0.04f;

		c_auto brightness = [](const float3& c) { return sqrt(dot(c * c, float3{ 0.299f, 0.587f, 0.114f })); };

		c_auto diffuse_rgb	  = float3{ diffuse.x, diffuse.y, diffuse.z };
		c_auto one_minus_spec = 1.f - max(max(specular.x, specular.y), specular.z);
		c_auto diffuse_b	  = brightness(diffuse_rgb);
		c_auto specular_b	  = brightness(specular);

		// solve metallic from perceived brightness: a*m^2 + b*m + k = 0
		auto metallic = 0.f;
		if (specular_b >= dielectric)
		{
			c_auto a = dielectric;
			c_auto b = diffuse_b * one_minus_spec / (1.f - dielectric) + specular_b - 2.f * dielectric;
			c_auto k = dielectric - specular_b;
			c_auto d = max(b * b - 4.f * a * k, 0.f);
			metallic = saturate((-b + sqrt(d)) / (2.f * a));
		}

		c_auto f0_dielectric = dielectric * (1.f - metallic);
		c_auto from_diffuse	 = diffuse_rgb * (one_minus_spec / (1.f - dielectric) / max(1.f - metallic, age::g::epsilon_1e6));
		c_auto from_specular = (specular - float3{ f0_dielectric, f0_dielectric, f0_dielectric }) / max(metallic, age::g::epsilon_1e6);
		c_auto base_color	 = saturate(lerp(from_diffuse, from_specular, metallic * metallic));

		return {
			float4{ base_color.x, base_color.y, base_color.z, diffuse.w },
			metallic,
			1.f - glossiness,
		};
	}

	e::material_import_warning_flags
	get_import_flags(e::gltf_material_parse_warning_flags flags) noexcept
	{
		using src = e::gltf_material_parse_warning_flags;
		using dst = e::material_import_warning_flags;

		auto res = dst::none;
		if (e::has_all(flags, src::spec_gloss_converted)) { res |= dst::spec_gloss_converted; }
		if (e::has_all(flags, src::texture_transform_dropped)) { res |= dst::texture_transform_dropped; }
		if (e::has_all(flags, src::texcoord_not_0_dropped)) { res |= dst::texcoord_not_0_dropped; }
		if (e::has_all(flags, src::extension_dropped)) { res |= dst::extension_dropped; }
		return res;
	}

	material_import_data
	generate_material_import_data(gltf_material_parse_data&& parse_data, const std::string& name) noexcept
	{
		auto res	= material_import_data{};
		res.enabled = true;

		res.warning_flags = get_import_flags(parse_data.warning_flags);
		res.name		  = util::to_fixed_str<config::max_asset_display_name_len>(name);

		res.base_color_factor  = parse_data.base_color_factor;
		res.metallic_factor	   = parse_data.metallic_factor;
		res.roughness_factor   = parse_data.roughness_factor;
		res.emissive_factor	   = parse_data.emissive_factor;
		res.normal_scale	   = parse_data.normal_scale;
		res.occlusion_strength = parse_data.occlusion_strength;
		res.alpha_cutoff	   = parse_data.alpha_cutoff;

		res.shading_model = parse_data.shading_model;
		res.double_sided  = parse_data.double_sided;

		res.base_color_sampler_kind			= parse_data.base_color_sampler_kind;
		res.metallic_roughness_sampler_kind = parse_data.metallic_roughness_sampler_kind;
		res.normal_sampler_kind				= parse_data.normal_sampler_kind;
		res.occlusion_sampler_kind			= parse_data.occlusion_sampler_kind;
		res.emissive_sampler_kind			= parse_data.emissive_sampler_kind;

		res.base_color_texture_idx		   = parse_data.base_color_texture_idx;
		res.metallic_roughness_texture_idx = parse_data.metallic_roughness_texture_idx;
		res.normal_texture_idx			   = parse_data.normal_texture_idx;
		res.occlusion_texture_idx		   = parse_data.occlusion_texture_idx;
		res.emissive_texture_idx		   = parse_data.emissive_texture_idx;

		return res;
	}
}	 // namespace age::asset::importer

// skeleton
namespace age::asset::importer
{
	e::skeleton_import_warning_flags
	get_import_flags(e::gltf_skeleton_parse_warning_flags gltf_warning_flags) noexcept
	{
		using src = e::gltf_skeleton_parse_warning_flags;
		using dst = e::skeleton_import_warning_flags;

		auto res = dst::none;
		if (e::has_all(gltf_warning_flags, src::non_joint_node_in_tree)) { res |= dst::non_joint_node_in_tree; }
		if (e::has_all(gltf_warning_flags, src::joint_decompose_failed)) { res |= dst::joint_decompose_failed; }
		if (e::has_all(gltf_warning_flags, src::empty_joint_name)) { res |= dst::joint_name_generated; }
		return res;
	}

	e::skeleton_import_error_flags
	get_import_flags(e::gltf_skeleton_parse_error_flags gltf_error_flags) noexcept
	{
		using src = e::gltf_skeleton_parse_error_flags;
		using dst = e::skeleton_import_error_flags;

		auto res = dst::none;
		if (e::has_all(gltf_error_flags, src::duplicated_joint_name)) { res |= dst::duplicated_joint_name; }
		return res;
	}

	skeleton_import_data
	generate_skeleton_import_data(gltf_skeleton_parse_data&& parse_data, const std::string& name) noexcept
	{
		auto res	= skeleton_import_data{};
		res.enabled = true;

		res.warning_flags = get_import_flags(parse_data.warning_flags);
		res.error_flags	  = get_import_flags(parse_data.error_flags);

		res.name = util::to_fixed_str<config::max_asset_display_name_len>(name);

		res.joint_name_vec.reserve(parse_data.joint_name_vec.size());
		for (auto&& joint_name : parse_data.joint_name_vec)
		{
			res.joint_name_vec.emplace_back(util::to_fixed_str<config::max_joint_name_len>(std::move(joint_name)));
		}
		parse_data.joint_name_vec = {};

		res.joint_vec.reserve(parse_data.joint_vec.size());

		for (auto& joint : parse_data.joint_vec)
		{
			res.joint_vec.emplace_back(skeleton_joint_import_data{
				.parent_idx	   = joint.parent_idx,
				.subtree_count = 0,
				.translation   = joint.translation,
				.rotation	   = joint.rotation,
				.scale		   = joint.scale,
			});
		}
		parse_data.joint_vec = {};

		for (c_auto& child_joint : res.joint_vec | std::views::reverse)
		{
			if (runtime::is_invalid_idx(child_joint.parent_idx) is_false)
			{
				res.joint_vec[child_joint.parent_idx].subtree_count += child_joint.subtree_count + 1;
			}
		}

		return res;
	}
}	 // namespace age::asset::importer

// meshes
namespace age::asset::importer
{
	e::mesh_import_warning_flags
	get_import_flags(e::gltf_mesh_parse_warning_flags gltf_warning_flags) noexcept
	{
		using src = e::gltf_mesh_parse_warning_flags;
		using dst = e::mesh_import_warning_flags;

		auto res = dst::none;
		if (e::has_all(gltf_warning_flags, src::uv1_dropped)) { res |= dst::uv1_dropped; }
		if (e::has_all(gltf_warning_flags, src::color_dropped)) { res |= dst::color_dropped; }
		if (e::has_all(gltf_warning_flags, src::custom_attribute_dropped)) { res |= dst::custom_attribute_dropped; }
		if (e::has_all(gltf_warning_flags, src::joints_over_4_merged)) { res |= dst::joints_over_4_merged; }
		if (e::has_all(gltf_warning_flags, src::blend_shape_uv_dropped)) { res |= dst::blend_shape_uv_dropped; }
		if (e::has_all(gltf_warning_flags, src::vertex_skin_zero_weight)) { res |= dst::vertex_skin_zero_weight; }
		if (e::has_all(gltf_warning_flags, src::skin_without_skeleton)) { res |= dst::skin_without_skeleton; }
		if (e::has_all(gltf_warning_flags, src::unskinned_submesh_rigid)) { res |= dst::unskinned_submesh_rigid; }
		if (e::has_all(gltf_warning_flags, src::empty_joint_name)) { res |= dst::joint_name_generated; }
		if (e::has_all(gltf_warning_flags, src::skin_dropped_by_multiple_skins)) { res |= dst::gltf_skin_dropped_by_multiple_skins; }
		return res;
	}

	e::mesh_import_error_flags
	get_import_flags(e::gltf_mesh_parse_error_flags gltf_error_flags) noexcept
	{
		using src = e::gltf_mesh_parse_error_flags;
		using dst = e::mesh_import_error_flags;

		auto res = dst::none;
		if (e::has_all(gltf_error_flags, src::draco_compressed)) { res |= dst::draco_compressed; }
		if (e::has_all(gltf_error_flags, src::meshopt_compressed)) { res |= dst::meshopt_compressed; }
		if (e::has_all(gltf_error_flags, src::unsupported_topology)) { res |= dst::unsupported_topology; }
		if (e::has_all(gltf_error_flags, src::position_missing)) { res |= dst::position_missing; }
		if (e::has_all(gltf_error_flags, src::vertex_idx_out_of_range)) { res |= dst::vertex_idx_out_of_range; }
		if (e::has_all(gltf_error_flags, src::blend_shape_count_mismatch)) { res |= dst::blend_shape_count_mismatch; }
		if (e::has_all(gltf_error_flags, src::joint_idx_out_of_range)) { res |= dst::joint_idx_out_of_range; }
		return res;
	}

	mesh_baked_import_data
	generate_mesh_baked_import_data(gltf_mesh_baked_parse_data&& parse_data, const std::string& name) noexcept
	{
		auto res		  = mesh_baked_import_data{};
		res.enabled		  = true;
		res.warning_flags = get_import_flags(parse_data.warning_flags);
		res.error_flags	  = get_import_flags(parse_data.error_flags);
		res.name		  = util::to_fixed_str<config::max_asset_display_name_len>(name);


		res.joint_name_vec.reserve(parse_data.joint_name_vec.size());
		for (auto&& joint_name : parse_data.joint_name_vec)
		{
			res.joint_name_vec.emplace_back(util::to_fixed_str<config::max_joint_name_len>(std::move(joint_name)));
		}
		parse_data.joint_name_vec = {};

		res.mesh_to_joint_vec = std::move(parse_data.mesh_to_joint_vec);

		res.blend_shape_name_vec.reserve(parse_data.blend_shape_name_vec.size());
		for (auto&& blend_shape_name : parse_data.blend_shape_name_vec)
		{
			res.blend_shape_name_vec.emplace_back(util::to_fixed_str<config::max_blend_shape_name_len>(std::move(blend_shape_name)));
		}
		parse_data.blend_shape_name_vec = {};

		res.blend_shape_weight_vec = std::move(parse_data.blend_shape_weight_vec);

		res.submesh_vec.reserve(parse_data.submesh_vec.size());
		for (auto& submesh : parse_data.submesh_vec)
		{
			res.submesh_vec.emplace_back(submesh_import_data{
				.index_buffer		= std::move(submesh.index_buffer),
				.vertex_buffer		= std::move(submesh.vertex_buffer),
				.vertex_skin_buffer = std::move(submesh.vertex_skin_buffer),
				.blend_shape_vec	= std::move(submesh.blend_shape_vec),
				.has_normal			= submesh.has_normal,
				.has_tangent		= submesh.has_tangent,
				.has_uv				= submesh.has_uv,
				.raster_mode		= submesh.raster_mode,
				.rt_alpha_test_mode = submesh.rt_alpha_test_mode,
				.rt_bake_mode		= submesh.rt_bake_mode,
			});
		}

		parse_data.submesh_vec = {};

		return res;
	}
}	 // namespace age::asset::importer

// model
namespace age::asset::importer
{
	e::model_import_warning_flags
	get_import_flags(e::gltf_model_parse_warning_flags gltf_warning_flags) noexcept
	{
		using src = e::gltf_model_parse_warning_flags;
		using dst = e::model_import_warning_flags;

		auto res = dst::none;
		if (e::has_all(gltf_warning_flags, src::material_variants_dropped)) { res |= dst::material_variants_dropped; }
		return res;
	}

	e::model_import_error_flags
	get_import_flags(e::gltf_model_parse_error_flags gltf_error_flags) noexcept
	{
		using src = e::gltf_model_parse_error_flags;
		using dst = e::model_import_error_flags;

		auto res = dst::none;
		return res;
	}

	model_import_data
	generate_model_import_data(gltf_model_parse_data&& parse_data, const std::string& name) noexcept
	{
		auto res		  = model_import_data{};
		res.enabled		  = true;
		res.warning_flags = get_import_flags(parse_data.warning_flags);
		res.error_flags	  = get_import_flags(parse_data.error_flags);
		res.name		  = util::to_fixed_str<config::max_asset_display_name_len>(name);

		res.mesh_idx				 = parse_data.mesh_idx;
		res.submesh_material_idx_vec = std::move(parse_data.submesh_gltf_material_idx_vec);

		return res;
	}
}	 // namespace age::asset::importer

// gltf
namespace age::asset::importer
{
	bool
	gltf_parse_data::is_valid() const noexcept
	{
		return error == e::gltf_parse_error_kind::none;
	}

	gltf_parse_data
	parse_gltf(std::string_view gltf_full_path) noexcept
	{
		auto res = gltf_parse_data{};

		if (fs::file_exists(gltf_full_path) is_false)
		{
			res.src_full_path = gltf_full_path;
			res.error		  = e::gltf_parse_error_kind::file_not_found;
			return res;
		}

		external::cgltf::load(gltf_full_path, res);
		return res;
	}
}	 // namespace age::asset::importer

// gltf import
namespace age::asset::importer::detail
{
	e::entity_import_warning_flags
	get_import_flags(e::gltf_entity_parse_warning_flags gltf_warning_flags) noexcept
	{
		using src = e::gltf_entity_parse_warning_flags;
		using dst = e::entity_import_warning_flags;

		auto res = dst::none;
		if (e::has_all(gltf_warning_flags, src::decompose_trs_failed)) { res |= dst::decompose_trs_failed; }
		if (e::has_all(gltf_warning_flags, src::instancing_dropped)) { res |= dst::instancing_dropped; }
		if (e::has_all(gltf_warning_flags, src::joint_attach_dropped_by_skin_mismatch)) { res |= dst::joint_attach_dropped_by_skin_mismatch; }
		if (e::has_all(gltf_warning_flags, src::skin_on_static_mesh)) { res |= dst::skin_on_static_mesh; }
		return res;
	}

	decltype(auto)
	gen_item_name(const import_data& res, const std::string& name, auto kind_tag, auto nth) noexcept
	{
		return name.empty() ? std::format("{}_{}_{}", res.asset_name.data(), kind_tag, nth) : name;
	}

	void
	import_gltf_textures(import_data& res, age::vector<gltf_texture_parse_data>&& parse_vec) noexcept
	{
		res.texture_import_data_vec.reserve(parse_vec.size());
		for (auto& vec = res.texture_import_data_vec;
			 auto&& [i, parse] : parse_vec | views::enumerate<uint32>)
		{
			vec.emplace_back(generate_texture_import_data(
				std::move(parse), gen_item_name(res, parse.name, "texture", i)));
		}

		parse_vec = age::vector<gltf_texture_parse_data>{};
	}

	void
	import_gltf_materials(import_data& res, age::vector<gltf_material_parse_data>&& parse_vec) noexcept
	{
		res.material_import_data_vec.reserve(parse_vec.size());
		for (auto& vec = res.material_import_data_vec;
			 auto&& [i, parse] : parse_vec | views::enumerate<uint32>)
		{
			c_auto& mat = vec.emplace_back(generate_material_import_data(
				std::move(parse), gen_item_name(res, parse.name, "material", i)));

			if (runtime::is_invalid_idx(mat.base_color_texture_idx) is_false)
			{
				res.texture_import_data_vec[mat.base_color_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::base_color);
			}
			if (runtime::is_invalid_idx(mat.metallic_roughness_texture_idx) is_false)
			{
				res.texture_import_data_vec[mat.metallic_roughness_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::metallic_roughness);
			}
			if (runtime::is_invalid_idx(mat.normal_texture_idx) is_false)
			{
				res.texture_import_data_vec[mat.normal_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::normal);
			}
			if (runtime::is_invalid_idx(mat.occlusion_texture_idx) is_false)
			{
				res.texture_import_data_vec[mat.occlusion_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::occlusion);
			}
			if (runtime::is_invalid_idx(mat.emissive_texture_idx) is_false)
			{
				res.texture_import_data_vec[mat.emissive_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::emissive);
			}
		}

		parse_vec = age::vector<gltf_material_parse_data>{};
	}

	void
	import_gltf_skeletons(import_data& res, age::vector<gltf_skeleton_parse_data>&& parse_vec) noexcept
	{
		res.skeleton_import_data_vec.reserve(parse_vec.size());
		for (auto& vec = res.skeleton_import_data_vec;
			 auto&& [i, parse] : parse_vec | views::enumerate<uint32>)
		{
			vec.emplace_back(generate_skeleton_import_data(
				std::move(parse), gen_item_name(res, parse.name, "skeleton", i)));
		}

		parse_vec = age::vector<gltf_skeleton_parse_data>{};
	}

	void
	import_gltf_meshes(import_data& res, age::vector<gltf_mesh_baked_parse_data>&& parse_vec) noexcept
	{
		res.mesh_import_data_vec.reserve(parse_vec.size());
		for (auto& vec = res.mesh_import_data_vec;
			 auto&& [i, parse] : parse_vec | views::enumerate<uint32>)
		{
			vec.emplace_back(generate_mesh_baked_import_data(
				std::move(parse), gen_item_name(res, parse.name, "mesh", i)));
		}

		parse_vec = age::vector<gltf_mesh_baked_parse_data>{};
	}

	void
	import_gltf_models(import_data& res, age::vector<gltf_model_parse_data>&& parse_vec) noexcept
	{
		res.model_import_data_vec.reserve(parse_vec.size());
		for (auto& vec = res.model_import_data_vec;
			 auto&& [i, parse] : parse_vec | views::enumerate<uint32>)
		{
			c_auto& model = vec.emplace_back(generate_model_import_data(
				std::move(parse), gen_item_name(res, std::string{}, "model", i)));

			if (runtime::is_invalid_idx(model.mesh_idx) or model.submesh_material_idx_vec.is_empty()) { continue; }

			for (auto&& [submesh_idx, submesh, mat] : std::views::zip(views::loop(res.mesh_import_data_vec[model.mesh_idx].submesh_vec.size<uint32>()),
																	  res.mesh_import_data_vec[model.mesh_idx].submesh_vec,
																	  model.submesh_material_idx_vec | views::idx_to(res.material_import_data_vec)))
			{
				if (runtime::is_invalid_idx(mat.base_color_texture_idx)) { continue; }
				auto& tex = res.texture_import_data_vec[mat.base_color_texture_idx];

				tex.model_submesh_reference_vec.emplace_back(i, submesh_idx);
			}
		}

		parse_vec = age::vector<gltf_model_parse_data>{};
	}

	void
	import_gltf_scenes(import_data& res, gltf_parse_data&& gltf_parse) noexcept
	{
		using entity_error	 = e::entity_import_error_flags;
		using entity_warning = e::entity_import_warning_flags;
		using scene_warning	 = e::scene_import_warning_flags;

		res.scene_import_data_vec.reserve(gltf_parse.gltf_scene_parse_data_vec.size());
		// local : scene local
		// local_entity_idx : scene local entity idx

		// entity_idx_map[local_ent_idx] == new_local_ent_idx
		// if entity_idx_map[local_ent_idx] is_invalid, this entity is erased
		// e.g. entity is joint_attach without child or components
		auto entity_idx_map = age::vector<uint32>{};
		// world_transform_mat_vec[local_ent_idx] == world_transform
		auto world_transform_mat_vec = age::vector<float3x4>{};
		for (auto& vec = res.scene_import_data_vec;
			 auto&& [i, scene_parse] : gltf_parse.gltf_scene_parse_data_vec | views::enumerate<uint32>)
		{
			auto& scene_import		 = vec.emplace_back();
			scene_import.name		 = util::to_fixed_str<config::max_scene_name_len>(gen_item_name(res, scene_parse.name, "scene", i));
			scene_import.enabled	 = i == (runtime::is_invalid_idx(gltf_parse.default_scene_idx) ? 0 : gltf_parse.default_scene_idx);
			scene_import.instantiate = scene_import.enabled;

			entity_idx_map.resize(scene_parse.entity_count);
			world_transform_mat_vec.resize(scene_parse.entity_count);

			c_auto entity_parse_span = std::span(gltf_parse.gltf_entity_parse_data_vec).subspan(scene_parse.entity_begin, scene_parse.entity_count);

			// first loop : fill basic components, init entity_idx_map
			for (auto&& [local_entity_idx, entity_parse] : entity_parse_span | views::enumerate<uint32>)
			{
				// check if to instantiate this entity
				c_auto is_joint_only = runtime::is_invalid_idx(entity_parse.skeleton_joint_idx) is_false
								   and runtime::is_invalid_idx(entity_parse.light_idx)
								   and runtime::is_invalid_idx(entity_parse.camera_idx)
								   and runtime::is_invalid_idx(entity_parse.model_idx)
								   and runtime::is_invalid_idx(entity_parse.skeleton_idx);
				AGE_ASSERT((is_joint_only and entity_parse.blend_shape_weight_override_arr.is_not_empty()) is_false, "if model_idx is_invalid, entity should not have blend_shape_override_arr");
				entity_idx_map[local_entity_idx] = is_joint_only ? age::get_invalid_idx<uint32>() : local_entity_idx;
				c_auto local_transform_mat		 = math::compose_trs(entity_parse.translation, entity_parse.rotation, entity_parse.scale);

				c_auto local_parent						  = runtime::is_invalid_idx(entity_parse.parent_idx) ? age::get_invalid_idx<uint32>() : entity_parse.parent_idx - scene_parse.entity_begin;
				world_transform_mat_vec[local_entity_idx] = runtime::is_invalid_idx(local_parent) ? local_transform_mat : math::affine_mul(world_transform_mat_vec[local_parent], local_transform_mat);

				auto& entity_import			= scene_import.entity_vec.emplace_back();
				entity_import.is_skinned	= entity_parse.is_skinned;
				entity_import.name			= util::to_fixed_str<config::max_entity_name_len>(entity_parse.name);
				entity_import.warning_flags = get_import_flags(entity_parse.warning_flags);

				entity_import.light_idx							  = age::get_invalid_idx<uint32>();
				entity_import.camera_idx						  = age::get_invalid_idx<uint32>();
				entity_import.model_idx							  = age::get_invalid_idx<uint32>();
				entity_import.parent_idx						  = age::get_invalid_idx<uint32>();
				entity_import.skeleton_idx						  = entity_parse.skeleton_idx;
				entity_import.joint_attach_idx					  = age::get_invalid_idx<uint32>();
				entity_import.blend_shape_weight_override_arr_idx = age::get_invalid_idx<uint32>();
				entity_import.translation						  = entity_parse.translation;
				entity_import.rotation							  = entity_parse.rotation;
				entity_import.scale								  = entity_parse.scale;
				entity_import.enabled							  = true;


				if (runtime::is_invalid_idx(entity_parse.model_idx) is_false)
				{
					entity_import.model_idx = entity_parse.model_idx;
				}
				if (runtime::is_invalid_idx(entity_parse.light_idx) is_false)
				{
					c_auto& light_parse		= gltf_parse.gltf_light_parse_data_vec[entity_parse.light_idx];
					entity_import.light_idx = scene_import.light_data_vec.size<uint32>();
					scene_import.light_data_vec.emplace_back(
						scene_import_data::light_data{
							.enabled	 = true,
							.kind		 = light_parse.kind,
							.cast_shadow = true,
							.range		 = light_parse.range,
							.direction	 = rotate(entity_parse.rotation, age::g::forward),
							.intensity	 = light_parse.intensity,
							.color		 = light_parse.color,
							.cos_inner	 = cos(light_parse.inner_cone_angle),
							.cos_outer	 = cos(light_parse.outer_cone_angle),
						});
				}
				if (runtime::is_invalid_idx(entity_parse.camera_idx) is_false)
				{
					c_auto& camera_parse	 = gltf_parse.gltf_camera_parse_data_vec[entity_parse.camera_idx];
					entity_import.camera_idx = scene_import.camera_data_vec.size<uint32>();
					scene_import.camera_data_vec.emplace_back(
						scene_import_data::camera_data{
							.enabled	  = true,
							.kind		  = camera_parse.is_perspective ? graphics::e::camera_kind::perspective : graphics::e::camera_kind::orthographic,
							.euler_deg	  = quat_to_euler_deg(entity_parse.rotation),
							.near_z		  = camera_parse.z_near,
							.far_z		  = camera_parse.z_far != 0.f ? camera_parse.z_far : 2000.f,
							.fov_y		  = camera_parse.yfov_or_ymag,
							.aspect_ratio = camera_parse.aspect,
							.view_width	  = 2.f * camera_parse.yfov_or_ymag * camera_parse.aspect,
							.view_height  = 2.f * camera_parse.yfov_or_ymag,
						});
				}

				if (entity_parse.blend_shape_weight_override_arr.is_not_empty())
				{
					entity_import.blend_shape_weight_override_arr_idx = scene_import.blend_shape_weight_override_arr_vec.size<uint32>();
					scene_import.blend_shape_weight_override_arr_vec.emplace_back(std::move(entity_parse.blend_shape_weight_override_arr));
				}
			}

			// second loop : reparent + joint_attach + gen_child_vec.
			{
				// pre-pass
				// skinned_mesh will be reparented. this may make a cycle if the skinned mesh entity is an ancestor of the skeleton owner entity
				// prepass detects this and set force_root flags to true
				// main loop will set skeleton owner as a root entity
				// if that entity has joint_attach, it will be ignored
				// if that entity is a skinned_model of a different skeleton, it will be ignored
				c_auto to_local_idx = [](c_auto& scene_parse, uint32 parse_entity_idx) { return parse_entity_idx - scene_parse.entity_begin; };
				auto   force_root	= age::dynamic_array<bool>::gen_sized_copy(scene_parse.entity_count, false);
				for (auto&& [local_entity_idx, entity_parse] : entity_parse_span | views::enumerate<uint32>)
				{
					if (entity_parse.is_skinned is_false) { continue; }

					c_auto skeleton_owner_local_entity_idx = to_local_idx(scene_parse, entity_parse.skeleton_owner_entity_idx);
					if (skeleton_owner_local_entity_idx == local_entity_idx) { continue; }

					for (auto parent_entity_idx = entity_parse_span[skeleton_owner_local_entity_idx].parent_idx;
						 runtime::is_invalid_idx(parent_entity_idx) is_false;
						 parent_entity_idx = entity_parse_span[to_local_idx(scene_parse, parent_entity_idx)].parent_idx)
					{
						// skinned mesh is the ancestor of the skeleton owner
						if (to_local_idx(scene_parse, parent_entity_idx) == local_entity_idx)
						{
							force_root[skeleton_owner_local_entity_idx] = true;
							break;
						}
					}
				}

				// main loop : reparent + joint_attach + gen_child_vec.
				for (auto&& [local_entity_idx, entity_parse] : entity_parse_span | views::enumerate<uint32>)
				{
					// entity is erased
					if (runtime::is_invalid_idx(entity_idx_map[local_entity_idx])) { continue; }

					c_auto set_parent = [](auto& scene_import, uint32 local_entity_idx, uint32 parent_local_entity_idx) {
						scene_import.entity_vec[local_entity_idx].parent_idx = parent_local_entity_idx;
						scene_import.entity_vec[parent_local_entity_idx].child_idx_vec.emplace_back(local_entity_idx);
					};

					c_auto add_joint_attach = [](auto& scene_import, c_auto& entity_parse_span, c_auto& gltf_parse, uint32 local_entity_idx, uint32 skeleton_owner_local_entity_idx, uint32 joint_idx) {
						c_auto	skeleton_idx = entity_parse_span[skeleton_owner_local_entity_idx].skeleton_idx;
						c_auto& joint_name	 = gltf_parse.gltf_skeleton_parse_data_vec[skeleton_idx].joint_name_vec[joint_idx];

						scene_import.entity_vec[local_entity_idx].joint_attach_idx = scene_import.joint_attach_data_vec.size<uint32>();
						scene_import.joint_attach_data_vec.emplace_back(scene_import_data::joint_attach_data{ .joint_name = util::to_fixed_str<config::max_joint_name_len>(joint_name) });
					};

					auto& entity_import = scene_import.entity_vec[local_entity_idx];

					if (force_root[local_entity_idx])
					{
						// skinned_mesh - skeleton_owner cycle detected
						// set skeleton owner as root entity

						AGE_ASSERT(runtime::is_invalid_idx(entity_parse.parent_idx) is_false);
						auto&& [success, t, r, s] = simd::load(world_transform_mat_vec[local_entity_idx]) | simd::decompose_trs();
						if (success)
						{
							entity_import.translation = t | simd::to<float3>();
							entity_import.rotation	  = r | simd::to<float4>();
							entity_import.scale		  = s | simd::to<float3>();
						}
						else
						{
							entity_import.warning_flags |= entity_warning::decompose_trs_failed;
						}
						entity_import.warning_flags	   |= entity_warning::forced_root_by_skinned_mesh_cycle;
						entity_import.joint_attach_idx	= age::get_invalid_idx<uint32>();
						entity_import.parent_idx		= age::get_invalid_idx<uint32>();

						if (runtime::is_invalid_idx(entity_parse.skeleton_joint_idx) is_false)
						{
							entity_import.warning_flags |= entity_warning::joint_attach_dropped_by_skinned_mesh_cycle;
						}

						if (entity_parse.is_skinned and to_local_idx(scene_parse, entity_parse.skeleton_owner_entity_idx) != local_entity_idx)
						{
							entity_import.model_idx		 = age::get_invalid_idx<uint32>();
							entity_import.is_skinned	 = false;
							entity_import.warning_flags |= entity_warning::skinned_model_dropped_by_skinned_mesh_cycle;
						}
						continue;
					}

					// add joint_attach, transform is set to identity (skeleton joint already carries it)
					// it is not an empty joint, this entity carries something (otherwise, entity would have been deleted)
					if (runtime::is_invalid_idx(entity_parse.skeleton_joint_idx) is_false)
					{
						AGE_ASSERT(to_local_idx(scene_parse, entity_parse.skeleton_owner_entity_idx) != local_entity_idx, "a joint can never be the owner of its own skeleton");
						c_auto skeleton_owner_local_entity_idx = to_local_idx(scene_parse, entity_parse.skeleton_owner_entity_idx);
						entity_import.translation			   = float3::zero();
						entity_import.rotation				   = float4{ 0, 0, 0, 1.f };
						entity_import.scale					   = float3::one();
						set_parent(scene_import, local_entity_idx, skeleton_owner_local_entity_idx);
						add_joint_attach(scene_import, entity_parse_span, gltf_parse, local_entity_idx, skeleton_owner_local_entity_idx, entity_parse.skeleton_joint_idx);
						continue;
					}

					// skinned mesh bound to another entity's skeleton: re-parent to the skeleton owner entity, keep the original world transform
					AGE_ASSERT((entity_parse.is_skinned and runtime::is_invalid_idx(entity_parse.skeleton_owner_entity_idx)) is_false, "if skinned, always has skeleton owner");
					if (entity_parse.is_skinned and to_local_idx(scene_parse, entity_parse.skeleton_owner_entity_idx) != local_entity_idx)
					{
						c_auto skeleton_owner_local_entity_idx = to_local_idx(scene_parse, entity_parse.skeleton_owner_entity_idx);

						if (entity_parse_span[skeleton_owner_local_entity_idx].scale.is_any_zero())
						{
							entity_import.warning_flags |= entity_warning::skeleton_owner_scale_zero;	 // inv(owner_world) undefined, local TRS kept as is
						}
						else
						{
							auto&& [success, t, r, s] = simd::mat_inv(simd::load(world_transform_mat_vec[skeleton_owner_local_entity_idx]))
													  | simd::mat_mul(simd::load(world_transform_mat_vec[local_entity_idx]))
													  | simd::decompose_trs();

							if (success)
							{
								entity_import.translation = t | simd::to<float3>();
								entity_import.rotation	  = r | simd::to<float4>();
								entity_import.scale		  = s | simd::to<float3>();
							}
							else
							{
								entity_import.warning_flags |= entity_warning::decompose_trs_failed;
							}
						}

						set_parent(scene_import, local_entity_idx, skeleton_owner_local_entity_idx);
						continue;
					}

					if (runtime::is_invalid_idx(entity_parse.parent_idx)) { continue; }

					c_auto parent_local_entity_idx = to_local_idx(scene_parse, entity_parse.parent_idx);
					if (runtime::is_invalid_idx(entity_idx_map[parent_local_entity_idx]) is_false)
					{
						set_parent(scene_import, local_entity_idx, parent_local_entity_idx);
						continue;
					}

					// parent was erased. the only erase rule today is a joint with nothing on it
					c_auto& parent_parse = entity_parse_span[parent_local_entity_idx];
					AGE_ASSERT(runtime::is_invalid_idx(parent_parse.skeleton_joint_idx) is_false, "erased entity must be a joint. a new erase rule needs its own reparent handling");

					c_auto skeleton_owner_local_entity_idx = to_local_idx(scene_parse, parent_parse.skeleton_owner_entity_idx);
					set_parent(scene_import, local_entity_idx, skeleton_owner_local_entity_idx);	// local TRS stays: it was relative to the joint node
					add_joint_attach(scene_import, entity_parse_span, gltf_parse, local_entity_idx, skeleton_owner_local_entity_idx, parent_parse.skeleton_joint_idx);
				}
			}

			// third loop : preorder reorder. erased entities are dropped, parent_idx / child_idx_vec are remapped, cycled entities are dropped
			{
				c_auto entity_count = scene_import.entity_vec.size<uint32>();

				auto root_idx_vec = age::vector<uint32>{};
				auto live_count	  = 0u;
				for (c_auto idx : views::loop(entity_count))
				{
					if (runtime::is_invalid_idx(entity_idx_map[idx])) { continue; }
					++live_count;
					if (runtime::is_invalid_idx(scene_import.entity_vec[idx].parent_idx)) { root_idx_vec.emplace_back(idx); }
				}

				// new_to_old_local_entity_idx_lut[new_local_ent_id] == old_local_ent_id.
				// entity_idx_map[old_local_ent_id] == new_local_ent_id, if invalid : erased (or in cycle)
				auto new_to_old_local_entity_idx_lut = age::vector<uint32>::gen_reserved(live_count);
				std::ranges::fill(entity_idx_map, age::get_invalid_idx<uint32>());
				util::for_each_preorder(
					root_idx_vec,
					[&](uint32 idx) -> auto& { return scene_import.entity_vec[idx].child_idx_vec; },
					[&](uint32 idx) { 
							entity_idx_map[idx] = new_to_old_local_entity_idx_lut.size<uint32>(); 
							new_to_old_local_entity_idx_lut.emplace_back(idx); });

				// cycle => no root => cannot reach from preorder traversal => new_count (new_to_old_size) < old_count (live_count)
				if (new_to_old_local_entity_idx_lut.size<uint32>() < live_count)
				{
					scene_import.warning_flags |= scene_warning::entity_hierarchy_cycle_dropped;
				}

				auto new_entity_vec = age::vector<entity_import_data>::gen_reserved(live_count);
				for (c_auto old_local_entity_idx : new_to_old_local_entity_idx_lut)
				{
					auto& ent = new_entity_vec.emplace_back(std::move(scene_import.entity_vec[old_local_entity_idx]));
					if (runtime::is_invalid_idx(ent.parent_idx) is_false)
					{
						ent.parent_idx = entity_idx_map[ent.parent_idx];
					}
					for (auto& child_idx : ent.child_idx_vec)
					{
						child_idx = entity_idx_map[child_idx];
					}
				}
				scene_import.entity_vec = std::move(new_entity_vec);
			}
		}
	}
}	 // namespace age::asset::importer::detail

namespace age::asset::importer::detail
{
	e::import_warning_flags
	get_import_flags(e::gltf_parse_warning_flags gltf_warning_flags) noexcept
	{
		using src = e::gltf_parse_warning_flags;
		using dst = e::import_warning_flags;

		auto res = dst::none;
		if (e::has_all(gltf_warning_flags, src::animation_dropped)) { res |= dst::animation_dropped; }
		if (e::has_all(gltf_warning_flags, src::entity_not_in_scene_dropped)) { res |= dst::entity_not_in_scene_dropped; }
		if (e::has_all(gltf_warning_flags, src::duplicated_entity_in_scene_dropped)) { res |= dst::duplicated_entity_in_scene_dropped; }
		if (e::has_all(gltf_warning_flags, src::skeleton_owner_node_not_in_scene)) { res |= dst::skeleton_owner_gltf_node_not_in_scene; }
		if (e::has_all(gltf_warning_flags, src::skeleton_owner_entity_generated)) { res |= dst::skeleton_owner_entity_generated; }
		return res;
	}

	e::import_error_flags
	get_import_flags(e::gltf_parse_error_kind gltf_error) noexcept
	{
		using src = e::gltf_parse_error_kind;
		using dst = e::import_error_flags;

		switch (gltf_error)
		{
		case src::none:
			return dst::none;
		case src::file_not_found:
			return dst::file_not_found;
		case src::invalid_format:
			return dst::invalid_format;
		case src::buffer_not_found:
			return dst::buffer_not_found;
		case src::internal:
			return dst::internal;
		default:
			return dst::internal;
		}
	}

	// marks name_collision, case-insensitive
	template <typename t_item>
	void
	resolve_common(import_data& data, age::vector<t_item>& import_data_vec, age::vector<uint32>& scratch_idx_vec) noexcept
	{
		scratch_idx_vec.clear();
		for (auto&& [i, item] : import_data_vec | views::enumerate<uint32>)
		{
			if (item.enabled is_false) { continue; }

			scratch_idx_vec.emplace_back(i);
		}

		c_auto name_of	  = [&](uint32 i) { return std::string_view{ import_data_vec[i].name.data() }; };
		c_auto equal	  = [&](uint32 l, uint32 r) { return std::ranges::equal(name_of(l), name_of(r), util::is_equal_ascii_ci); };
		c_auto less		  = [&](uint32 l, uint32 r) { return std::ranges::lexicographical_compare(name_of(l), name_of(r), util::is_less_ascii_ci); };
		c_auto mark_error = [&](uint32 i) { import_data_vec[i].error_flags |= BARE_OF(import_data_vec[i].error_flags)::name_collision; };

		std::ranges::sort(scratch_idx_vec, less);

		for (auto chunk : scratch_idx_vec | std::views::chunk_by(equal))
		{
			if (chunk.size() > 1)
			{
				for (auto idx : chunk)
				{
					mark_error(idx);
				}
			}
		}
	}

	template <asset::e::kind asset_kind, typename t_item>
	void
	resolve_common(import_data& data, age::vector<t_item>& import_data_vec, age::vector<uint32>& scratch_idx_vec, std::string_view target_dir) noexcept
	{
		scratch_idx_vec.clear();
		c_auto suffix_size = std::strlen(config::asset_extension) + std::strlen(asset::get_asset_tag<asset_kind>());
		for (auto&& [i, item] : import_data_vec | views::enumerate<uint32>)
		{
			if (item.enabled is_false) { continue; }

			c_auto name_str = fs::join(target_dir, item.name.data());
			if (name_str.size() + suffix_size >= config::max_asset_path_len)
			{
				item.error_flags |= BARE_OF(item.error_flags)::name_too_long;
				continue;
			}

			if (fs::file_exists(fs::join(asset::get_root_dir(), asset::get_asset_full_path<asset_kind>(name_str))))
			{
				item.warning_flags |= BARE_OF(item.warning_flags)::file_already_exists;
			}
			scratch_idx_vec.emplace_back(i);
		}

		c_auto name_of	  = [&](uint32 i) { return std::string_view{ import_data_vec[i].name.data() }; };
		c_auto equal	  = [&](uint32 l, uint32 r) { return std::ranges::equal(name_of(l), name_of(r), util::is_equal_ascii_ci); };
		c_auto less		  = [&](uint32 l, uint32 r) { return std::ranges::lexicographical_compare(name_of(l), name_of(r), util::is_less_ascii_ci); };
		c_auto mark_error = [&](uint32 i) { import_data_vec[i].error_flags |= BARE_OF(import_data_vec[i].error_flags)::name_collision; };

		std::ranges::sort(scratch_idx_vec, less);

		for (auto chunk : scratch_idx_vec | std::views::chunk_by(equal))
		{
			if (chunk.size() > 1)
			{
				for (auto idx : chunk)
				{
					mark_error(idx);
				}
			}
		}
	}
}	 // namespace age::asset::importer::detail

namespace age::asset::importer
{
	import_data
	generate_gltf_import_data(gltf_parse_data&& gltf_parse, std::string_view asset_name, std::string_view target_dir) noexcept
	{
		auto res = import_data{};

		// file, asset_name first because item name fallbacks use it
		res.src_full_path = std::move(gltf_parse.src_full_path);
		res.asset_name	  = util::to_fixed_str<config::max_asset_display_name_len>(asset_name);
		res.warning_flags = detail::get_import_flags(gltf_parse.warning_flags);
		res.error_flags	  = detail::get_import_flags(gltf_parse.error);

		// default dir
		res.texture_dir	 = util::to_fixed_str<config::max_asset_path_len>(fs::join(target_dir, fs::join(to_string(asset::e::kind::texture), res.asset_name.data())));
		res.material_dir = util::to_fixed_str<config::max_asset_path_len>(fs::join(target_dir, fs::join(to_string(asset::e::kind::material), res.asset_name.data())));
		res.mesh_dir	 = util::to_fixed_str<config::max_asset_path_len>(fs::join(target_dir, fs::join("mesh", res.asset_name.data())));
		res.skeleton_dir = util::to_fixed_str<config::max_asset_path_len>(fs::join(target_dir, fs::join("skeleton", res.asset_name.data())));
		res.model_dir	 = util::to_fixed_str<config::max_asset_path_len>(fs::join(target_dir, fs::join(to_string(asset::e::kind::model), res.asset_name.data())));
		res.scene_dir	 = util::to_fixed_str<config::max_asset_path_len>(fs::join(target_dir, fs::join("scene", res.asset_name.data())));

		detail::import_gltf_textures(res, std::move(gltf_parse.gltf_texture_parse_data_vec));
		detail::import_gltf_materials(res, std::move(gltf_parse.gltf_material_parse_data_vec));
		detail::import_gltf_skeletons(res, std::move(gltf_parse.gltf_skeleton_parse_data_vec));
		detail::import_gltf_meshes(res, std::move(gltf_parse.gltf_mesh_parse_data_vec));
		detail::import_gltf_models(res, std::move(gltf_parse.gltf_model_parse_data_vec));
		detail::import_gltf_scenes(res, std::move(gltf_parse));

		for (auto& import_texture : res.texture_import_data_vec)
		{
			fill_texture_default_import_data(import_texture);
		}
		return res;
	}

	void
	resolve_import(import_data& data) noexcept
	{
		auto scratch_idx_vec = age::vector<uint32>{};

		using tex_warning	   = e::texture_import_warning_flags;
		using tex_error		   = e::texture_import_error_flags;
		using mat_warning	   = e::material_import_warning_flags;
		using mat_error		   = e::material_import_error_flags;
		using mesh_warning	   = e::mesh_import_warning_flags;
		using mesh_error	   = e::mesh_import_error_flags;
		using model_warning	   = e::model_import_warning_flags;
		using model_error	   = e::model_import_error_flags;
		using skeleton_warning = e::skeleton_import_warning_flags;
		using skeleton_error   = e::skeleton_import_error_flags;
		using scene_warning	   = e::scene_import_warning_flags;
		using scene_error	   = e::scene_import_error_flags;
		using entity_warning   = e::entity_import_warning_flags;
		using entity_error	   = e::entity_import_error_flags;

		for (auto& tex : data.texture_import_data_vec)
		{
			tex.usage_flags	   = e::texture_usage_flags::none;
			tex.error_flags	  &= ~(tex_error::name_collision | tex_error::name_too_long);
			tex.warning_flags &= ~(tex_warning::alpha_dropped_by_format | tex_warning::alpha_missing_in_source | tex_warning::referenced_as_srgb_and_linear
								   | tex_warning::referenced_with_different_alpha_cutoff | tex_warning::file_already_exists);
			tex.material_reference_vec.clear();
			tex.model_submesh_reference_vec.clear();
		}
		for (auto& mat : data.material_import_data_vec)
		{
			mat.warning_flags &= ~(mat_warning::file_already_exists);
			mat.error_flags	  &= ~(mat_error::texture_disabled | mat_error::name_collision | mat_error::name_too_long);

			mat.model_reference_vec.clear();
		}
		for (auto& mesh : data.mesh_import_data_vec)
		{
			mesh.warning_flags &= ~(mesh_warning::file_already_exists | mesh_warning::duplicated_blend_shape_name | mesh_warning::empty_blend_shape_name);
			mesh.error_flags   &= ~(mesh_error::name_collision | mesh_error::name_too_long | mesh_error::duplicated_joint_name | mesh_error::empty_joint_name);

			mesh.model_reference_vec.clear();
		}
		for (auto& skeleton : data.skeleton_import_data_vec)
		{
			skeleton.warning_flags &= ~(skeleton_warning::file_already_exists);
			skeleton.error_flags	= skeleton_error::none;
		}
		for (auto& model : data.model_import_data_vec)
		{
			model.warning_flags &= ~(model_warning::file_already_exists | model_warning::material_without_submesh);
			model.error_flags	 = model_error::none;
		}
		for (auto& scene : data.scene_import_data_vec)
		{
			scene.warning_flags &= ~(scene_warning::file_already_exists);
			scene.error_flags	&= ~(scene_error::name_collision | scene_error::name_too_long);

			scene.has_hierarchy					  = false;
			scene.has_model						  = false;
			scene.has_skinned_model				  = false;
			scene.has_skeleton					  = false;
			scene.has_directional_light			  = false;
			scene.has_point_light				  = false;
			scene.has_spot_light				  = false;
			scene.has_blend_shape_weight_override = false;

			for (auto& entity : scene.entity_vec)
			{
				entity.warning_flags &= ~(entity_warning::skin_on_static_mesh | entity_warning::skinned_but_model_missing | entity_warning::skinned_but_mesh_missing | entity_warning::skinned_and_skeleton_disabled);
				entity.error_flags	 &= ~(entity_error::none);
			}
		}
		detail::resolve_common<asset::e::kind::texture>(data, data.texture_import_data_vec, scratch_idx_vec, data.texture_dir.data());
		detail::resolve_common<asset::e::kind::material>(data, data.material_import_data_vec, scratch_idx_vec, data.material_dir.data());
		detail::resolve_common<asset::e::kind::mesh_baked>(data, data.mesh_import_data_vec, scratch_idx_vec, data.mesh_dir.data());
		detail::resolve_common<asset::e::kind::model>(data, data.model_import_data_vec, scratch_idx_vec, data.model_dir.data());
		detail::resolve_common(data, data.skeleton_import_data_vec, scratch_idx_vec);
		detail::resolve_common(data, data.scene_import_data_vec, scratch_idx_vec);

		// material
		for (auto&& [i, mat] : data.material_import_data_vec | views::enumerate<uint32>)
		{
			if (runtime::is_invalid_idx(mat.base_color_texture_idx) is_false)
			{
				mat.error_flags |= data.texture_import_data_vec[mat.base_color_texture_idx].enabled ? mat_error::none : mat_error::texture_disabled;
				data.texture_import_data_vec[mat.base_color_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::base_color);
			}
			if (runtime::is_invalid_idx(mat.metallic_roughness_texture_idx) is_false)
			{
				mat.error_flags |= data.texture_import_data_vec[mat.metallic_roughness_texture_idx].enabled ? mat_error::none : mat_error::texture_disabled;
				data.texture_import_data_vec[mat.metallic_roughness_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::metallic_roughness);
			}
			if (runtime::is_invalid_idx(mat.normal_texture_idx) is_false)
			{
				mat.error_flags |= data.texture_import_data_vec[mat.normal_texture_idx].enabled ? mat_error::none : mat_error::texture_disabled;
				data.texture_import_data_vec[mat.normal_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::normal);
			}
			if (runtime::is_invalid_idx(mat.occlusion_texture_idx) is_false)
			{
				mat.error_flags |= data.texture_import_data_vec[mat.occlusion_texture_idx].enabled ? mat_error::none : mat_error::texture_disabled;
				data.texture_import_data_vec[mat.occlusion_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::occlusion);
			}
			if (runtime::is_invalid_idx(mat.emissive_texture_idx) is_false)
			{
				mat.error_flags |= data.texture_import_data_vec[mat.emissive_texture_idx].enabled ? mat_error::none : mat_error::texture_disabled;
				data.texture_import_data_vec[mat.emissive_texture_idx].material_reference_vec.emplace_back(i, e::texture_usage_flags::emissive);
			}
		}

		// mesh
		for (auto&& [i, mesh] : data.mesh_import_data_vec | views::enumerate<uint32>)
		{
			// joint name
			{
				c_auto name_of = [&](uint32 i) { return std::string_view{ mesh.joint_name_vec[i].data() }; };
				c_auto equal   = [&](uint32 l, uint32 r) { return std::ranges::equal(name_of(l), name_of(r)); };
				c_auto less	   = [&](uint32 l, uint32 r) { return std::ranges::lexicographical_compare(name_of(l), name_of(r)); };

				scratch_idx_vec.clear();
				for (auto&& [nth_joint, joint_name] : mesh.joint_name_vec | views::enumerate<uint32>)
				{
					if (joint_name[0] == '\0')
					{
						mesh.error_flags |= mesh_error::empty_joint_name;
					}

					scratch_idx_vec.emplace_back(nth_joint);
				}

				std::ranges::sort(scratch_idx_vec, less);

				for (auto chunk : scratch_idx_vec | std::views::chunk_by(equal))
				{
					if (chunk.size() > 1)
					{
						mesh.error_flags |= mesh_error::duplicated_joint_name;
						break;
					}
				}
			}

			// blend shape name
			{
				c_auto name_of = [&](uint32 i) { return std::string_view{ mesh.blend_shape_name_vec[i].data() }; };
				c_auto equal   = [&](uint32 l, uint32 r) { return std::ranges::equal(name_of(l), name_of(r)); };
				c_auto less	   = [&](uint32 l, uint32 r) { return std::ranges::lexicographical_compare(name_of(l), name_of(r)); };

				scratch_idx_vec.clear();
				for (auto&& [nth_blend, blend_shape_name] : mesh.blend_shape_name_vec | views::enumerate<uint32>)
				{
					if (blend_shape_name[0] == '\0')
					{
						mesh.warning_flags |= mesh_warning::empty_blend_shape_name;
					}

					scratch_idx_vec.emplace_back(nth_blend);
				}

				std::ranges::sort(scratch_idx_vec, less);

				for (auto chunk : scratch_idx_vec | std::views::chunk_by(equal))
				{
					if (chunk.size() > 1)
					{
						mesh.warning_flags |= mesh_warning::duplicated_blend_shape_name;
						break;
					}
				}
			}
		}

		// skeleton
		for (auto&& [i, skeleton] : data.skeleton_import_data_vec | views::enumerate<uint32>)
		{
			c_auto name_of = [&](uint32 i) { return std::string_view{ skeleton.joint_name_vec[i].data() }; };
			c_auto equal   = [&](uint32 l, uint32 r) { return std::ranges::equal(name_of(l), name_of(r)); };
			c_auto less	   = [&](uint32 l, uint32 r) { return std::ranges::lexicographical_compare(name_of(l), name_of(r)); };

			scratch_idx_vec.clear();
			for (auto&& [nth_joint, joint_name] : skeleton.joint_name_vec | views::enumerate<uint32>)
			{
				if (joint_name[0] == '\0')
				{
					skeleton.error_flags |= skeleton_error::empty_joint_name;
				}

				scratch_idx_vec.emplace_back(nth_joint);
			}


			std::ranges::sort(scratch_idx_vec, less);

			for (auto chunk : scratch_idx_vec | std::views::chunk_by(equal))
			{
				if (chunk.size() > 1)
				{
					skeleton.error_flags |= skeleton_error::duplicated_joint_name;
					break;
				}
			}
		}

		// model
		for (auto&& [i, model] : data.model_import_data_vec | views::enumerate<uint32>)
		{
			if (runtime::is_invalid_idx(model.mesh_idx) is_false)
			{
				auto& mesh_import = data.mesh_import_data_vec[model.mesh_idx];
				for (auto&& [submesh_idx, submesh, mat_idx] : std::views::zip(views::loop(mesh_import.submesh_vec.size<uint32>()),
																			  mesh_import.submesh_vec,
																			  model.submesh_material_idx_vec))
				{
					if (runtime::is_invalid_idx(mat_idx)) { continue; }
					c_auto& mat = data.material_import_data_vec[mat_idx];

					if (runtime::is_invalid_idx(mat.base_color_texture_idx)) { continue; }
					auto& tex = data.texture_import_data_vec[mat.base_color_texture_idx];

					tex.model_submesh_reference_vec.emplace_back(i, submesh_idx);
				}

				model.error_flags |= mesh_import.enabled ? model_error::none : model_error::mesh_disabled;
				mesh_import.model_reference_vec.emplace_back(i);

				if (mesh_import.submesh_vec.size() < model.submesh_material_idx_vec.size())
				{
					model.warning_flags |= model_warning::material_without_submesh;
				}
				if (model.submesh_material_idx_vec.size() < mesh_import.submesh_vec.size())
				{
					model.error_flags |= model_error::submesh_without_material;
				}
			}

			for (const auto&& [nth_mat, mat_idx] : model.submesh_material_idx_vec | views::enumerate<uint32>)
			{
				if (runtime::is_invalid_idx(mat_idx) is_false)
				{
					model.error_flags |= data.material_import_data_vec[mat_idx].enabled ? model_error::none : model_error::material_disabled;
					data.material_import_data_vec[mat_idx].model_reference_vec.emplace_back(i, nth_mat);
				}
			}
		}

		// texture
		for (auto& tex : data.texture_import_data_vec)
		{
			using enum e::texture_usage_flags;

			auto alpha_cutoff = -1.f;

			auto need_alpha_channel = false;
			for (c_auto& [ model_idx, submesh_idx ] : tex.model_submesh_reference_vec)
			{
				c_auto& model	= data.model_import_data_vec[model_idx];
				c_auto& mesh	= data.mesh_import_data_vec[model.mesh_idx];
				c_auto& submesh = mesh.submesh_vec[submesh_idx];

				if (need_alpha_channel = submesh.need_alpha_channel())
				{
					break;
				}
			}

			for (c_auto& [ mat_idx, usage ] : tex.material_reference_vec)
			{
				c_auto& mat		 = data.material_import_data_vec[mat_idx];
				tex.usage_flags |= usage;

				if (has_all(usage, alpha_mask) and alpha_cutoff != mat.alpha_cutoff)
				{
					tex.warning_flags |= tex_warning::referenced_with_different_alpha_cutoff;
				}

				alpha_cutoff = mat.alpha_cutoff;
			}
			c_auto is_srgb	 = has_any(tex.usage_flags, base_color | emissive | ui_sprite);
			c_auto is_linear = has_any(tex.usage_flags, metallic_roughness | normal | font_msdf | lut);
			if (is_srgb and is_linear)
			{
				tex.warning_flags |= tex_warning::referenced_as_srgb_and_linear;
			}

			tex.bake_option.alpha_threshold = alpha_cutoff;
			tex.bake_option.keep_coverage	= alpha_cutoff;

			if (tex.has_alpha is_false and need_alpha_channel)
			{
				tex.warning_flags |= tex_warning::alpha_missing_in_source;
			}

			if (need_alpha_channel and graphics::texture_format_has_alpha(tex.bake_option.format) is_false)
			{
				tex.warning_flags |= tex_warning::alpha_dropped_by_format;
			}
		}

		// scene
		for (auto& scene : data.scene_import_data_vec)
		{
			for (auto& entity : scene.entity_vec)
			{
				if (runtime::is_invalid_idx(entity.parent_idx) is_false)
				{
					scene.has_hierarchy = true;
				}
				if (runtime::is_invalid_idx(entity.model_idx) is_false)
				{
					if (entity.is_skinned)
					{
						scene.has_skinned_model = true;
					}
					else
					{
						scene.has_model = true;
					}
				}
				if (runtime::is_invalid_idx(entity.skeleton_idx) is_false)
				{
					scene.has_skeleton = true;
				}
				if (runtime::is_invalid_idx(entity.light_idx) is_false)
				{
					c_auto& light = scene.light_data_vec[entity.light_idx];
					if (light.kind == graphics::e::light_kind::directional)
					{
						scene.has_directional_light = true;
					}
					else if (light.kind == graphics::e::light_kind::point)
					{
						scene.has_point_light = true;
					}
					else if (light.kind == graphics::e::light_kind::spot)
					{
						scene.has_spot_light = true;
					}
				}
				if (runtime::is_invalid_idx(entity.blend_shape_weight_override_arr_idx) is_false)
				{
					scene.has_blend_shape_weight_override = true;
				}

				// joint attach check
				[&] {
					if (runtime::is_invalid_idx(entity.joint_attach_idx)) { return; }

					c_auto& joint_attach = scene.joint_attach_data_vec[entity.joint_attach_idx];
					if (joint_attach.enabled is_false) { return; }

					AGE_ASSERT(runtime::is_invalid_idx(entity.parent_idx) is_false, "if joint attached, parent_entity should always exists");
					auto& skeleton_owner_entity = scene.entity_vec[entity.parent_idx];

					if (runtime::is_invalid_idx(skeleton_owner_entity.skeleton_idx))
					{
						skeleton_owner_entity.error_flags |= entity_error::joint_attach_skeleton_missing;
						return;
					}

					c_auto& owner_skeleton = data.skeleton_import_data_vec[skeleton_owner_entity.skeleton_idx];

					if (owner_skeleton.enabled is_false)
					{
						skeleton_owner_entity.error_flags |= entity_error::referenced_skeleton_is_disabled;
						entity.error_flags				  |= entity_error::owner_skeleton_disabled;
						return;
					}

					if (std::ranges::contains(owner_skeleton.joint_name_vec | std::views::transform([](c_auto& arr) { return std::string_view{ arr.data() }; }), std::string_view{ joint_attach.joint_name.data() }, {}) is_false)
					{
						entity.error_flags |= entity_error::joint_attach_skeleton_mismatch;
					}
				}();

				// skinned model check
				[&] {
					c_auto is_mesh_bindable_to_skeleton = [](c_auto& skeleton_joint_name_vec, c_auto& mesh_joint_name_vec) {
						c_auto to_sv		  = [](c_auto& name) { return std::string_view{ name.data() }; };
						c_auto skeleton_names = skeleton_joint_name_vec | std::views::transform(to_sv);
						return std::ranges::all_of(mesh_joint_name_vec | std::views::transform(to_sv),
												   [&](std::string_view mesh_joint_name) { return std::ranges::contains(skeleton_names, mesh_joint_name); });
					};

					if (entity.is_skinned is_false) { return; }

					if (runtime::is_invalid_idx(entity.model_idx))
					{
						entity.warning_flags |= entity_warning::skinned_but_model_missing;
						return;
					}

					c_auto& model = data.model_import_data_vec[entity.model_idx];

					if (model.enabled is_false)
					{
						entity.warning_flags |= entity_warning::skinned_but_model_missing;
						return;
					}

					if (runtime::is_invalid_idx(model.mesh_idx))
					{
						entity.warning_flags |= entity_warning::skinned_but_mesh_missing;
						return;
					}

					c_auto& mesh = data.mesh_import_data_vec[model.mesh_idx];

					if (mesh.joint_name_vec.is_empty())
					{
						entity.warning_flags |= entity_warning::skin_on_static_mesh;
						return;
					}

					if (runtime::is_invalid_idx(entity.skeleton_idx) is_false)
					{
						c_auto& skeleton = data.skeleton_import_data_vec[entity.skeleton_idx];

						if (skeleton.enabled is_false)
						{
							entity.warning_flags |= entity_warning::skinned_and_skeleton_disabled;
							return;
						}

						if (is_mesh_bindable_to_skeleton(skeleton.joint_name_vec, mesh.joint_name_vec) is_false)
						{
							entity.error_flags |= entity_error::skinned_model_skeleton_mismatch;
						}
						return;
					}

					// skeleton owner is entity's parent

					if (runtime::is_invalid_idx(entity.parent_idx))
					{
						entity.error_flags |= entity_error::skinned_but_skeleton_missing;
						return;
					}

					auto& skeleton_owner_entity = scene.entity_vec[entity.parent_idx];

					if (runtime::is_invalid_idx(skeleton_owner_entity.skeleton_idx))
					{
						entity.error_flags |= entity_error::skinned_but_skeleton_missing;
						return;
					}

					c_auto& owner_skeleton = data.skeleton_import_data_vec[skeleton_owner_entity.skeleton_idx];

					if (owner_skeleton.enabled is_false)
					{
						skeleton_owner_entity.error_flags |= entity_error::referenced_skeleton_is_disabled;
						entity.error_flags				  |= entity_error::owner_skeleton_disabled;
						return;
					}

					if (is_mesh_bindable_to_skeleton(owner_skeleton.joint_name_vec, mesh.joint_name_vec) is_false)
					{
						entity.error_flags |= entity_error::skinned_model_skeleton_mismatch;
					}
				}();

				if (entity.scale.is_any_zero())
				{
					entity.error_flags |= entity_error::scale_is_zero;
				}
			}
		}

		{
			data.has_warning		  = data.warning_flags != e::import_warning_flags::none;
			data.has_tex_warning	  = std::ranges::any_of(data.texture_import_data_vec, [](c_auto& tex) { return tex.warning_flags != tex_warning::none; });
			data.has_mat_warning	  = std::ranges::any_of(data.material_import_data_vec, [](c_auto& mat) { return mat.warning_flags != mat_warning::none; });
			data.has_mesh_warning	  = std::ranges::any_of(data.mesh_import_data_vec, [](c_auto& mesh) { return mesh.warning_flags != mesh_warning::none; });
			data.has_model_warning	  = std::ranges::any_of(data.model_import_data_vec, [](c_auto& model) { return model.warning_flags != model_warning::none; });
			data.has_skeleton_warning = std::ranges::any_of(data.skeleton_import_data_vec, [](c_auto& skeleton) { return skeleton.warning_flags != skeleton_warning::none; });
			data.has_scene_warning	  = false;
			data.has_entity_warning	  = false;
			for (c_auto& scene : data.scene_import_data_vec)
			{
				data.has_scene_warning |= scene.warning_flags != scene_warning::none;

				for (c_auto& entity : scene.entity_vec)
				{
					if (data.has_entity_warning = entity.warning_flags != entity_warning::none) { break; }
				}
			}

			data.has_warning = data.warning_flags != e::import_warning_flags::none
							or data.has_tex_warning
							or data.has_mat_warning
							or data.has_mesh_warning
							or data.has_model_warning
							or data.has_skeleton_warning
							or data.has_scene_warning
							or data.has_entity_warning;
		}

		{
			data.has_tex_error		= std::ranges::any_of(data.texture_import_data_vec, [](c_auto& tex) { return tex.error_flags != tex_error::none; });
			data.has_mat_error		= std::ranges::any_of(data.material_import_data_vec, [](c_auto& mat) { return mat.error_flags != mat_error::none; });
			data.has_mesh_error		= std::ranges::any_of(data.mesh_import_data_vec, [](c_auto& mesh) { return mesh.error_flags != mesh_error::none; });
			data.has_model_error	= std::ranges::any_of(data.model_import_data_vec, [](c_auto& model) { return model.error_flags != model_error::none; });
			data.has_skeleton_error = std::ranges::any_of(data.skeleton_import_data_vec, [](c_auto& skeleton) { return skeleton.error_flags != skeleton_error::none; });
			data.has_scene_error	= false;
			data.has_entity_error	= false;
			for (c_auto& scene : data.scene_import_data_vec)
			{
				data.has_scene_error |= scene.error_flags != scene_error::none;

				for (c_auto& entity : scene.entity_vec)
				{
					if (data.has_entity_error = entity.error_flags != entity_error::none) { break; }
				}
			}

			data.has_error = data.error_flags != e::import_error_flags::none
						  or data.has_tex_error
						  or data.has_mat_error
						  or data.has_mesh_error
						  or data.has_model_error
						  or data.has_skeleton_error
						  or data.has_scene_error
						  or data.has_entity_error;
		}
	}
}	 // namespace age::asset::importer

namespace age::asset::importer::detail
{
	std::string
	get_commit_error_message(e::commit_error_kind error_kind, auto&&... arg)
	{
		switch (error_kind)
		{
		case age::asset::importer::e::commit_error_kind::commit_temporary_dir_not_empty:
		{
			return std::format("directory {} should be left empty or deleted", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::commit_temporary_dir_create_failed:
		{
			return std::format("commit_temporary_dir_create_failed, directory path : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::create_target_dir_failed:
		{
			return std::format("create_target_dir_failed, target dir : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::tex_temporary_file_create_failed:
		{
			return std::format("tex_temporary_file_create_failed, tex_name : {}, temporary file name : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::tex_bake_failed:
		{
			return std::format("tex_bake_failed, tex_name : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::mesh_build_failed:
		{
			return std::format("mesh_build_failed, mesh_name : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::temp_file_cleanup_failed:
		{
			return std::format("temp_file_cleanup_failed, temporary file name : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::temp_dir_cleanup_failed:
		{
			return std::format("temp_dir_cleanup_failed, temporary dir name : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::target_path_cannot_be_overwritten:
		{
			return std::format("target_path_cannot_be_overwritten, target_path : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::backup_failed:
		{
			return std::format("backup of the existing file failed, failed file [original, backup] : {}", FWD(arg)...);
		}
		case age::asset::importer::e::commit_error_kind::move_failed:
		{
			return std::format("failed to move the new file to its target path, failed file [original, backup] : {}", FWD(arg)...);
		}
		default:
		{
			AGE_UNREACHABLE("invalid error_kind : {}", to_idx(error_kind));
		}
		}
	}
}	 // namespace age::asset::importer::detail

namespace age::asset::importer
{
	commit_result
	commit_import(const import_data& data) noexcept
	{
		using error = e::commit_error_kind;
		auto res	= commit_result{};

		constexpr auto temp_dir_name	   = ".__import_commit_temp__/";
		c_auto		   temp_directory_path = fs::join(asset::get_root_dir(), temp_dir_name);

		if (fs::dir_exists(temp_directory_path))
		{
			if (fs::is_dir_empty(temp_directory_path) is_false)
			{
				res.error	= error::commit_temporary_dir_not_empty;
				res.message = detail::get_commit_error_message(res.error, std::move(temp_directory_path));
				return res;
			}
		}
		else if (fs::create_dir(temp_directory_path) is_false)
		{
			res.error	= error::commit_temporary_dir_create_failed;
			res.message = detail::get_commit_error_message(res.error, std::move(temp_directory_path));
			return res;
		}

		auto tex_temp_path_vec		= age::vector<std::string>::gen_reserved(data.texture_import_data_vec.size());
		auto material_temp_path_vec = age::vector<std::string>::gen_reserved(data.material_import_data_vec.size());
		auto mesh_temp_path_vec		= age::vector<std::string>::gen_reserved(data.mesh_import_data_vec.size());
		auto model_temp_path_vec	= age::vector<std::string>::gen_reserved(data.model_import_data_vec.size());
		auto skeleton_temp_path_vec = age::vector<std::string>::gen_reserved(data.skeleton_import_data_vec.size());
		auto scene_temp_path_vec	= age::vector<std::string>::gen_reserved(data.scene_import_data_vec.size());

		auto tex_target_path_vec	  = age::vector<std::string>::gen_reserved(data.texture_import_data_vec.size());
		auto material_target_path_vec = age::vector<std::string>::gen_reserved(data.material_import_data_vec.size());
		auto mesh_target_path_vec	  = age::vector<std::string>::gen_reserved(data.mesh_import_data_vec.size());
		auto model_target_path_vec	  = age::vector<std::string>::gen_reserved(data.model_import_data_vec.size());
		auto skeleton_target_path_vec = age::vector<std::string>::gen_reserved(data.skeleton_import_data_vec.size());
		auto scene_target_path_vec	  = age::vector<std::string>::gen_reserved(data.scene_import_data_vec.size());

		// full_path
		c_auto target_tex_dir	   = fs::join(asset::get_root_dir(), data.texture_dir.data());
		c_auto target_material_dir = fs::join(asset::get_root_dir(), data.material_dir.data());
		c_auto target_mesh_dir	   = fs::join(asset::get_root_dir(), data.mesh_dir.data());
		c_auto target_model_dir	   = fs::join(asset::get_root_dir(), data.model_dir.data());
		c_auto target_skeleton_dir = fs::join(asset::get_root_dir(), data.skeleton_dir.data());
		c_auto target_scene_dir	   = fs::join(asset::get_root_dir(), data.scene_dir.data());

		bool has_texture_enabled  = false;
		bool has_material_enabled = false;
		bool has_mesh_enabled	  = false;
		bool has_model_enabled	  = false;
		bool has_skeleton_enabled = false;
		bool has_scene_enabled	  = false;

		for (c_auto& tex : data.texture_import_data_vec | std::views::filter(&texture_import_data::enabled))
		{
			has_texture_enabled = true;
			c_auto& rel_path	= res.tex_path_vec.emplace_back(std::string{ asset::get_asset_full_path<asset::e::kind::texture>(fs::join(data.texture_dir, tex.name)).data() });
			tex_target_path_vec.emplace_back(fs::join(asset::get_root_dir(), rel_path));
		}
		for (c_auto& mat : data.material_import_data_vec | std::views::filter(&material_import_data::enabled))
		{
			has_material_enabled = true;
			c_auto& rel_path	 = res.material_path_vec.emplace_back(std::string{ asset::get_asset_full_path<asset::e::kind::material>(fs::join(data.material_dir, mat.name)).data() });
			material_target_path_vec.emplace_back(fs::join(asset::get_root_dir(), rel_path));
		}
		for (c_auto& mesh : data.mesh_import_data_vec | std::views::filter(&mesh_baked_import_data::enabled))
		{
			has_mesh_enabled = true;
			c_auto& rel_path = res.mesh_path_vec.emplace_back(std::string{ asset::get_asset_full_path<asset::e::kind::mesh_baked>(fs::join(data.mesh_dir, mesh.name)).data() });
			mesh_target_path_vec.emplace_back(fs::join(asset::get_root_dir(), rel_path));
		}
		for (c_auto& model : data.model_import_data_vec | std::views::filter(&model_import_data::enabled))
		{
			has_model_enabled = true;
			c_auto& rel_path  = res.model_path_vec.emplace_back(std::string{ asset::get_asset_full_path<asset::e::kind::model>(fs::join(data.model_dir, model.name)).data() });
			model_target_path_vec.emplace_back(fs::join(asset::get_root_dir(), rel_path));
		}
		for (c_auto& skeleton : data.skeleton_import_data_vec | std::views::filter(&skeleton_import_data::enabled))
		{
			has_skeleton_enabled = true;
			// c_auto& rel_path	 = res.skeleton_path_vec.emplace_back(std::string{ asset::get_asset_full_path<asset::e::kind::skeleton>(fs::join(data.skeleton_dir, skeleton.name)).data() });
			// skeleton_target_path_vec.emplace_back(fs::join(asset::get_root_dir(), rel_path));
		}
		for (c_auto& scene : data.scene_import_data_vec | std::views::filter(&scene_import_data::enabled))
		{
			has_scene_enabled = true;
			// c_auto& rel_path  = res.scene_path_vec.emplace_back(std::string{ asset::get_asset_full_path<asset::e::kind::scene>(fs::join(data.scene_dir, scene.name)).data() });
			// scene_target_path_vec.emplace_back(fs::join(asset::get_root_dir(), rel_path));
		}

		// create target directories
		{
			if (has_texture_enabled and fs::create_dir(target_tex_dir) is_false)
			{
				res.error	= error::create_target_dir_failed;
				res.message = detail::get_commit_error_message(res.error, target_tex_dir);
				fs::remove_dir(temp_directory_path);
				return res;
			}
			if (has_material_enabled and fs::create_dir(target_material_dir) is_false)
			{
				res.error	= error::create_target_dir_failed;
				res.message = detail::get_commit_error_message(res.error, target_material_dir);
				fs::remove_dir(temp_directory_path);
				return res;
			}
			if (has_mesh_enabled and fs::create_dir(target_mesh_dir) is_false)
			{
				res.error	= error::create_target_dir_failed;
				res.message = detail::get_commit_error_message(res.error, target_mesh_dir);
				fs::remove_dir(temp_directory_path);
				return res;
			}
			if (has_model_enabled and fs::create_dir(target_model_dir) is_false)
			{
				res.error	= error::create_target_dir_failed;
				res.message = detail::get_commit_error_message(res.error, target_model_dir);
				fs::remove_dir(temp_directory_path);
				return res;
			}
			if (has_skeleton_enabled and fs::create_dir(target_skeleton_dir) is_false)
			{
				res.error	= error::create_target_dir_failed;
				res.message = detail::get_commit_error_message(res.error, target_skeleton_dir);
				fs::remove_dir(temp_directory_path);
				return res;
			}
			if (has_scene_enabled and fs::create_dir(target_scene_dir) is_false)
			{
				res.error	= error::create_target_dir_failed;
				res.message = detail::get_commit_error_message(res.error, target_scene_dir);
				fs::remove_dir(temp_directory_path);
				return res;
			}
		}


		// texture
		{
			auto tmp_file_path_vec	   = age::vector<std::string>{};
			auto tmp_file_path_ptr_vec = age::vector<const char*>{};


			for (const auto&& [i, tex] : data.texture_import_data_vec | views::enumerate<uint32>)
			{
				if (tex.enabled is_false) { continue; }

				tmp_file_path_vec.clear();
				tmp_file_path_ptr_vec.clear();
				tmp_file_path_vec.reserve(tex.source_bytes_vec.size());
				tmp_file_path_ptr_vec.reserve(tex.source_bytes_vec.size());

				for (const auto&& [j, bytes] : tex.source_bytes_vec | views::enumerate<uint32>)
				{
					tmp_file_path_vec.emplace_back(std::format("{}/__temp_tex__[{}][{}].{}", temp_directory_path, i, j, to_string(tex.file_kind)));
					tmp_file_path_ptr_vec.emplace_back(tmp_file_path_vec.back().data());

					if (fs::write_file(tmp_file_path_vec.back(), bytes) is_false)
					{
						res.error	= error::tex_temporary_file_create_failed;
						res.message = detail::get_commit_error_message(res.error, tex.name, tmp_file_path_vec.back());
						fs::remove_dir(temp_directory_path);
						return res;
					}
				}

				c_auto rel_path		= fs::join(temp_dir_name, "texture", tex.name);
				c_auto bake_success = asset::texture::bake(tmp_file_path_ptr_vec, rel_path, tex.bake_option);
				tex_temp_path_vec.emplace_back(fs::join(asset::get_root_dir(), std::move(rel_path)));

				if (bake_success is_false)
				{
					res.error	= error::tex_bake_failed;
					res.message = detail::get_commit_error_message(res.error, tex.name);
					fs::remove_dir(temp_directory_path);
					return res;
				}
				else
				{
					for (c_auto& temp_file : tmp_file_path_vec)
					{
						if (fs::remove_file(temp_file) is_false)
						{
							res.error	= error::temp_file_cleanup_failed;
							res.message = detail::get_commit_error_message(res.error, temp_file);
							fs::remove_dir(temp_directory_path);
							return res;
						}
					}
				}
			}
		}
		// material
		{
			c_auto get_texture_path_buf = [&](auto tex_idx) {
				if (runtime::is_invalid_idx(tex_idx))
				{
					return asset::path_buf{};
				}
				else
				{
					return asset::get_asset_full_path<asset::e::kind::texture>(std::format("{}/{}", data.texture_dir, data.texture_import_data_vec[tex_idx].name));
				}
			};

			for (c_auto& mat : data.material_import_data_vec)
			{
				if (mat.enabled is_false) { continue; }

				auto mat_desc = asset::material_file_desc{
					.base_color_factor				 = mat.base_color_factor,
					.metallic_factor				 = mat.metallic_factor,
					.roughness_factor				 = mat.roughness_factor,
					.emissive_factor				 = mat.emissive_factor,
					.normal_scale					 = mat.normal_scale,
					.occlusion_strength				 = mat.occlusion_strength,
					.alpha_cutoff					 = mat.alpha_cutoff,
					.double_sided					 = mat.double_sided,
					.shading_model					 = mat.shading_model,
					.base_color_sampler_kind		 = mat.base_color_sampler_kind,
					.metallic_roughness_sampler_kind = mat.metallic_roughness_sampler_kind,
					.normal_sampler_kind			 = mat.normal_sampler_kind,
					.occlusion_sampler_kind			 = mat.occlusion_sampler_kind,
					.emissive_sampler_kind			 = mat.emissive_sampler_kind,
					.tex_base_color_path			 = get_texture_path_buf(mat.base_color_texture_idx),
					.tex_metallic_roughness_path	 = get_texture_path_buf(mat.metallic_roughness_texture_idx),
					.tex_normal_path				 = get_texture_path_buf(mat.normal_texture_idx),
					.tex_occlusion_path				 = get_texture_path_buf(mat.occlusion_texture_idx),
					.tex_emissive_path				 = get_texture_path_buf(mat.emissive_texture_idx),
				};

				c_auto rel_path = std::format("{}/material/{}", temp_dir_name, mat.name);
				asset::material::build(rel_path, mat_desc);
				material_temp_path_vec.emplace_back(fs::join(asset::get_root_dir(), std::move(rel_path)));
			}
		}
		// mesh
		{
			auto submesh_desc_vec = age::vector<asset::submesh_desc>{};
			for (c_auto& mesh : data.mesh_import_data_vec)
			{
				if (mesh.enabled is_false) { continue; }

				submesh_desc_vec.clear();
				submesh_desc_vec.reserve(mesh.submesh_vec.size());

				for (c_auto& submesh : mesh.submesh_vec)
				{
					submesh_desc_vec.emplace_back(asset::submesh_desc{
						.vertex_buffer		= submesh.vertex_buffer,
						.index_buffer		= submesh.index_buffer,
						.raster_mode		= submesh.raster_mode,
						.rt_alpha_test_mode = submesh.rt_alpha_test_mode,
						.rt_bake_mode		= submesh.rt_bake_mode,
						.gen_normal			= submesh.has_normal is_false,
						.gen_tangent		= submesh.has_uv and (submesh.has_normal is_false or submesh.has_tangent is_false),
					});
				}

				c_auto desc = asset::mesh_baked_desc{
					// todo, support more vertex kind
					.v_kind = asset::e::vertex_kind::pnt_uv0,
					// todo, support more normal gen mode
					.normal_calc_mode = asset::e::normal_calc_mode_kind::flat,
					.submesh_span	  = submesh_desc_vec
				};

				c_auto rel_path = std::format("{}/mesh/{}", temp_dir_name, mesh.name);

				if (asset::mesh_baked::build(rel_path, desc) is_false)
				{
					res.error	= error::mesh_build_failed;
					res.message = detail::get_commit_error_message(res.error, mesh.name);
					fs::remove_dir(temp_directory_path);
					return res;
				}

				mesh_temp_path_vec.emplace_back(fs::join(asset::get_root_dir(), std::move(rel_path)));
			}
		}
		// model
		{
			c_auto get_mesh_path_buf = [&](auto mesh_idx) {
				if (runtime::is_invalid_idx(mesh_idx))
				{
					return asset::path_buf{};
				}
				else
				{
					return asset::get_asset_full_path<asset::e::kind::mesh_baked>(fs::join(data.mesh_dir, data.mesh_import_data_vec[mesh_idx].name));
				}
			};


			c_auto get_material_path_buf = [&](auto mat_idx) {
				if (runtime::is_invalid_idx(mat_idx))
				{
					return asset::path_buf{};
				}
				else
				{
					return asset::get_asset_full_path<asset::e::kind::material>(fs::join(data.material_dir, data.material_import_data_vec[mat_idx].name));
				}
			};

			for (c_auto& model : data.model_import_data_vec)
			{
				if (model.enabled is_false) { continue; }

				auto desc = asset::model_file_desc{
					.mesh_path_buf		   = get_mesh_path_buf(model.mesh_idx),
					.material_path_buf_vec = model.submesh_material_idx_vec | std::views::transform(get_material_path_buf) | std::ranges::to<age::vector<asset::path_buf>>()
				};

				c_auto rel_path = std::format("{}/model/{}", temp_dir_name, model.name);
				asset::model::build(rel_path, desc);
				model_temp_path_vec.emplace_back(fs::join(asset::get_root_dir(), std::move(rel_path)));
			}
		}

		// skeleton
		// scene, entity (scene_import_file?)

		c_auto target_path_span_arr = std::array{ std::span{ tex_target_path_vec },
												  std::span{ material_target_path_vec },
												  std::span{ mesh_target_path_vec },
												  std::span{ model_target_path_vec },
												  std::span{ skeleton_target_path_vec },
												  std::span{ scene_target_path_vec } };

		// move each file, if failed, unroll each file
		{
			// [original, backup]
			auto backup_file_path_vec = age::vector<std::tuple<std::string, std::string>>::gen_reserved(
				tex_target_path_vec.size()
				+ material_target_path_vec.size()
				+ mesh_target_path_vec.size()
				+ model_target_path_vec.size()
				+ skeleton_target_path_vec.size()
				+ scene_target_path_vec.size());
			// auto backup_file_path_vec = std::vector<std::tuple<std::string, std::string>>{};


			for (c_auto& path : target_path_span_arr | std::views::join)
			{
				if (fs::file_exists(path))
				{
					backup_file_path_vec.emplace_back(std::string{ path }, std::format("{}__backup__{}", temp_directory_path, fs::get_file_name(path)));
				}
			}

			// backup
			auto backup_count = 0u;
			for (const auto& [original, backup] : backup_file_path_vec)
			{
				if (fs::rename(original, backup) is_false) { break; }
				++backup_count;
			}

			// check backup failed
			if (backup_count != backup_file_path_vec.size<uint32>())
			{
				// backup failed
				auto rollback_failed_vec = age::vector<std::tuple<std::string, std::string>>::gen_reserved(backup_count);
				for (const auto& [original, backup] : backup_file_path_vec | std::views::take(backup_count))
				{
					if (fs::rename(backup, original) is_false)
					{
						rollback_failed_vec.emplace_back(std::string{ backup }, std::string{ original });
					}
				}

				if (rollback_failed_vec.is_not_empty())
				{
					res.error	 = error::backup_failed_and_rollback_failed;
					res.message	 = "backup failed and rollback also failed."
								   "the temp dir is kept so you can restore the backups to their original paths by hand.\n";
					res.message += std::format("backup failed file [original, backup] : {}\n", backup_file_path_vec[backup_count]);
					for (const auto& [backup, original] : rollback_failed_vec)
					{
						res.message += std::format("backup path : {}, original path : {}\n");
					}
					return res;
				}

				res.error	= error::backup_failed;
				res.message = detail::get_commit_error_message(res.error, backup_file_path_vec[backup_count]);
				fs::remove_dir(temp_directory_path);
				return res;
			}

			// move each files
			auto temp_path_span_arr = std::array{ std::span{ tex_temp_path_vec },
												  std::span{ material_temp_path_vec },
												  std::span{ mesh_temp_path_vec },
												  std::span{ model_temp_path_vec },
												  std::span{ skeleton_temp_path_vec },
												  std::span{ scene_temp_path_vec } };

			auto move_count				 = 0u;
			auto move_success			 = true;
			auto move_failed_temp_path	 = std::string{};
			auto move_failed_target_path = std::string{};
			for (const auto&& [from, to] : std::views::zip(temp_path_span_arr | std::views::join, target_path_span_arr | std::views::join))
			{
				if (fs::rename(from, to) is_false)
				{
					move_success			= false;
					move_failed_temp_path	= from;
					move_failed_target_path = to;
					break;
				}
				++move_count;
			}

			if (move_success is_false)
			{
				// rollback rename
				for (auto&& [from, to] : std::views::zip(temp_path_span_arr | std::views::join, target_path_span_arr | std::views::join) | std::views::take(move_count))
				{
					if (fs::rename(to, from) is_false)
					{
						fs::remove_file(to);
					}
				}

				// get backup and restore original

				auto rollback_failed_vec = age::vector<std::tuple<std::string, std::string>>::gen_reserved(backup_count);
				for (const auto& [original, backup] : backup_file_path_vec | std::views::take(backup_count))
				{
					if (fs::rename(backup, original) is_false)
					{
						rollback_failed_vec.emplace_back(std::string{ backup }, std::string{ original });
					}
				}

				if (rollback_failed_vec.is_not_empty())
				{
					res.error	 = error::move_failed_and_rollback_failed;
					res.message	 = "move failed and rollback also failed."
								   "the temp dir is kept so you can restore the backups to their original paths by hand. some new files may already be in place.\n";
					res.message += std::format("move failed temp file path : {}, target file path : {}\n", std::move(move_failed_temp_path), std::move(move_failed_target_path));
					for (const auto& [backup, original] : rollback_failed_vec)
					{
						res.message += std::format("backup path : {}, original path : {}\n", backup, original);
					}
					return res;
				}

				res.error	= error::move_failed;
				res.message = detail::get_commit_error_message(res.error, std::move(move_failed_temp_path), std::move(move_failed_target_path));
				fs::remove_dir(temp_directory_path);
				return res;
			}
		}

		// clear temp directory
		if (fs::remove_dir(temp_directory_path) is_false)
		{
			res.error	= error::temp_dir_cleanup_failed;
			res.message = detail::get_commit_error_message(res.error, temp_directory_path);
		}

		// fill_overwritten_asset_handle
		{
			c_auto fill_overwritten_asset_handle = [&](asset::e::kind e_kind, std::span<const std::string> rel_path_span) {
				for (c_auto& path : rel_path_span)
				{
					if (c_auto h_asset = asset::find(e_kind, path))
					{
						res.overwritten_asset_handle_vec.emplace_back(h_asset);
					}
				}
			};

			fill_overwritten_asset_handle(asset::e::kind::texture, res.tex_path_vec);
			fill_overwritten_asset_handle(asset::e::kind::material, res.material_path_vec);
			fill_overwritten_asset_handle(asset::e::kind::mesh_baked, res.mesh_path_vec);
			fill_overwritten_asset_handle(asset::e::kind::model, res.model_path_vec);
			// fill_overwritten_asset_handle(asset::e::kind::skeleton, skeleton_target_path_vec);
			// fill_overwritten_asset_handle(asset::e::kind::scene, scene_target_path_vec);
		}

		return res;
	}
}	 // namespace age::asset::importer