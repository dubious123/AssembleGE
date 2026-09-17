#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::cgltf::detail
{
	std::string
	make_string(name_view v) noexcept
	{
		return std::string(v.p, v.count);
	}

	std::string_view
	make_string_view(name_view v) noexcept
	{
		return std::string_view(v.p, v.count);
	}
}	 // namespace age::external::cgltf::detail

// texture
namespace age::external::cgltf::detail
{
	uint32
	get_image_idx(const gltf_data& data, int gltf_texture_idx) noexcept
	{
		if (gltf_texture_idx < 0)
		{
			return age::get_invalid_id<uint32>();
		}

		for (c_auto& tex_data = data.p_texture[gltf_texture_idx];
			 auto	 idx : { tex_data.image_idx, tex_data.webp_image_idx, tex_data.basisu_image_idx })
		{
			if (idx >= 0)
			{
				return idx;
			}
		}
		return age::get_invalid_id<uint32>();
	}

	graphics::e::sampler_kind
	get_sampler_kind(wrap_kind wrap_s, wrap_kind wrap_t, filter_kind mag, filter_kind min, mipmap_kind mip) noexcept
	{
		c_auto clamp   = wrap_s == wrap_kind::clamp_to_edge or wrap_t == wrap_kind::clamp_to_edge;
		c_auto nearest = mag == filter_kind::nearest or min == filter_kind::nearest;
		(void)mip;

		if (nearest)
		{
			return clamp ? graphics::e::sampler_kind::point_clamp : graphics::e::sampler_kind::point_wrap;
		}
		return clamp ? graphics::e::sampler_kind::linear_clamp : graphics::e::sampler_kind::linear_wrap;
	}

	graphics::e::sampler_kind
	get_sampler_kind(const gltf_data& data, int gltf_texture_idx) noexcept
	{
		if (gltf_texture_idx < 0)
		{
			return get_sampler_kind(wrap_kind::repeat, wrap_kind::repeat, filter_kind::undefined, filter_kind::undefined, mipmap_kind::none);
		}

		c_auto& tex_data = data.p_texture[gltf_texture_idx];
		return get_sampler_kind(tex_data.wrap_s, tex_data.wrap_t, tex_data.mag_filter, tex_data.min_filter, tex_data.mipmap);
	}

	void
	fill_textures(const gltf_data& data, asset::importer::gltf_parse_data& res) noexcept
	{
		using error = asset::importer::e::gltf_texture_parse_error_flags;

		c_auto dir = fs::get_parent_path(res.src_full_path);
		res.gltf_texture_parse_data_vec.resize(data.image_count);

		for (auto&& [img_data, parse_data] : std::views::zip(std::span(data.p_image, data.image_count), res.gltf_texture_parse_data_vec))
		{
			c_auto uri_stem = [](name_view uri) {
				c_auto sv = make_string_view(uri);
				if (sv.empty() or sv.starts_with("data:")) { return std::string{}; }
				return std::string{ fs::get_file_stem(sv) };
			};

			parse_data.name = img_data.name.count > 0 ? make_string(img_data.name) : uri_stem(img_data.uri);

			if (img_data.p_embedded)
			{
				parse_data.bytes = age::dynamic_array<std::byte>{ std::from_range_t{}, std::as_bytes(std::span(img_data.p_embedded, img_data.embedded_size)) };
			}
			else if (img_data.uri.p)
			{
				c_auto path = fs::join(dir, img_data.uri.p);

				if (fs::file_exists(path) is_false)
				{
					parse_data.error_flags |= error::image_file_not_found;
					continue;
				}

				auto buf = fs::read_file(path);

				if (buf.is_empty())
				{
					parse_data.error_flags |= error::image_file_unreadable;
					continue;
				}
				parse_data.bytes = age::dynamic_array<std::byte>{ buf.data(), buf.data() + buf.size() };
			}

			c_auto hint							   = img_data.uri.p ? make_string_view(img_data.uri) : make_string_view(img_data.mime_type);
			const auto&& [kind, extent, has_alpha] = asset::importer::get_texture_source_info({ parse_data.bytes }, hint);
			parse_data.file_kind				   = kind;
			parse_data.width					   = extent.width;
			parse_data.height					   = extent.height;
			parse_data.has_alpha				   = has_alpha;
		}
	}
}	 // namespace age::external::cgltf::detail

// materials
namespace age::external::cgltf::detail
{
	void
	fill_materials(const gltf_data& data, asset::importer::gltf_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_material_parse_warning_flags;

		res.gltf_material_parse_data_vec.resize(data.material_count);

		for (auto&& [mat_data, parse_data] : std::views::zip(std::span(data.p_material, data.material_count), res.gltf_material_parse_data_vec))
		{
			parse_data.name = make_string(mat_data.name);

			c_auto has_spec_gloss = (mat_data.feature_mask & material_feature_specular_glossiness) != 0;
			c_auto use_spec_gloss = has_spec_gloss and mat_data.has_pbr_metallic_roughness is_false;

			// factors
			if (use_spec_gloss)
			{
				const auto& [base_color, metallic, roughness]  = asset::importer::cvt_spec_gloss_to_mr(mat_data.specular_glossiness.diffuse_factor,
																									   mat_data.specular_glossiness.specular_factor,
																									   mat_data.specular_glossiness.glossiness_factor);
				parse_data.base_color_factor				   = base_color;
				parse_data.metallic_factor					   = metallic;
				parse_data.roughness_factor					   = roughness;
				parse_data.warning_flags					  |= warning::spec_gloss_converted;
			}
			else
			{
				parse_data.base_color_factor = mat_data.base_color_factor;
				parse_data.metallic_factor	 = mat_data.metallic_factor;
				parse_data.roughness_factor	 = mat_data.roughness_factor;
			}

			parse_data.emissive_factor	  = float3{ mat_data.emissive_factor } * mat_data.emissive_strength;
			parse_data.normal_scale		  = mat_data.normal_texture.scale;
			parse_data.occlusion_strength = mat_data.occlusion_texture.scale;
			parse_data.alpha_cutoff		  = mat_data.alpha_cutoff;
			parse_data.double_sided		  = mat_data.double_sided;
			parse_data.shading_model	  = (mat_data.feature_mask & material_feature_unlit) != 0
											  ? graphics::e::material_shading_model_kind::pbr_unlit
											  : graphics::e::material_shading_model_kind::pbr_default;

			auto handle_texture = [&](auto& slot) {
				auto texture_idx = get_image_idx(data, slot.texture_idx);
				auto sampler	 = graphics::e::sampler_kind::linear_wrap;

				if (texture_idx == age::get_invalid_id<uint32>())
				{
					return std::tuple{ texture_idx, sampler };
				}

				sampler = get_sampler_kind(data, slot.texture_idx);

				if (slot.texcoord != 0)
				{
					parse_data.warning_flags |= warning::texcoord_not_0_dropped;
					texture_idx				  = age::get_invalid_id<uint32>();
				}
				if (slot.transform.has)
				{
					parse_data.warning_flags |= warning::texture_transform_dropped;
				}

				return std::tuple{ texture_idx, sampler };
			};

			const auto&& [base_color_tex_id, base_color_sampler_kind]				  = handle_texture(use_spec_gloss ? mat_data.specular_glossiness.diffuse_texture : mat_data.base_color_texture);
			const auto&& [metallic_roughness_tex_id, metallic_roughness_sampler_kind] = handle_texture(use_spec_gloss ? mat_data.specular_glossiness.specular_glossiness_texture : mat_data.metallic_roughness_texture);
			const auto&& [normal_tex_id, normal_sampler_kind]						  = handle_texture(mat_data.normal_texture);
			const auto&& [occlusion_tex_id, occlusion_sampler_kind]					  = handle_texture(mat_data.occlusion_texture);
			const auto&& [emissive_tex_id, emissive_sampler_kind]					  = handle_texture(mat_data.emissive_texture);

			parse_data.base_color_texture_idx		  = base_color_tex_id;
			parse_data.metallic_roughness_texture_idx = metallic_roughness_tex_id;
			parse_data.normal_texture_idx			  = normal_tex_id;
			parse_data.occlusion_texture_idx		  = occlusion_tex_id;
			parse_data.emissive_texture_idx			  = emissive_tex_id;

			parse_data.base_color_sampler_kind		   = base_color_sampler_kind;
			parse_data.metallic_roughness_sampler_kind = metallic_roughness_sampler_kind;
			parse_data.normal_sampler_kind			   = normal_sampler_kind;
			parse_data.occlusion_sampler_kind		   = occlusion_sampler_kind;
			parse_data.emissive_sampler_kind		   = emissive_sampler_kind;

			parse_data.normal_scale		  = mat_data.normal_texture.scale;
			parse_data.occlusion_strength = mat_data.occlusion_texture.scale;

			if (use_spec_gloss and metallic_roughness_tex_id != age::get_invalid_id<uint32>())
			{
				res.gltf_texture_parse_data_vec[metallic_roughness_tex_id].is_spec_gloss = true;
			}

			constexpr auto handled = material_feature_unlit | material_feature_specular_glossiness | material_feature_emissive_strength;
			if (mat_data.feature_mask & ~handled)
			{
				parse_data.warning_flags |= warning::extension_dropped;
			}
		}
	}
}	 // namespace age::external::cgltf::detail

// lights
namespace age::external::cgltf::detail
{
	graphics::e::light_kind
	cvt_light_kind(detail::light_kind kind) noexcept
	{
		switch (kind)
		{
		case age::external::cgltf::detail::light_kind::directional:
			return graphics::e::light_kind::directional;
		case age::external::cgltf::detail::light_kind::point:
			return graphics::e::light_kind::point;
		case age::external::cgltf::detail::light_kind::spot:
			return graphics::e::light_kind::spot;
		default:
			AGE_ASSERT(false, "invalid light_kind {}", std::to_underlying(kind));
			return graphics::e::light_kind::point;
		}
	}

	void
	fill_lights(const gltf_data& data, asset::importer::gltf_parse_data& res) noexcept
	{
		res.gltf_light_parse_data_vec.resize(data.light_count);

		for (auto&& [light_data, parse_data] : std::views::zip(std::span(data.p_light, data.light_count), res.gltf_light_parse_data_vec))
		{
			parse_data.kind				= cvt_light_kind(light_data.kind);
			parse_data.color			= float3{ light_data.color };
			parse_data.intensity		= light_data.intensity;
			parse_data.range			= light_data.range;	   // 0 = infinite, same as glTF
			parse_data.inner_cone_angle = light_data.spot_inner_cone_angle;
			parse_data.outer_cone_angle = light_data.spot_outer_cone_angle;
		}
	}
}	 // namespace age::external::cgltf::detail

// cameras
namespace age::external::cgltf::detail
{
	void
	fill_cameras(const gltf_data& data, asset::importer::gltf_parse_data& res) noexcept
	{
		res.gltf_camera_parse_data_vec.resize(data.camera_count);

		for (auto&& [camera_data, parse_data] : std::views::zip(std::span(data.p_camera, data.camera_count), res.gltf_camera_parse_data_vec))
		{
			parse_data.is_perspective = camera_data.kind == camera_kind::perspective;
			parse_data.z_near		  = camera_data.znear;

			if (parse_data.is_perspective)
			{
				parse_data.yfov_or_ymag = camera_data.yfov;
				parse_data.aspect		= camera_data.has_aspect_ratio ? camera_data.aspect_ratio : 0.f;	// 0 = unspecified, viewport decides
				parse_data.z_far		= camera_data.has_zfar ? camera_data.zfar : 0.f;					// 0 = infinite
			}
			else
			{
				parse_data.yfov_or_ymag = camera_data.ymag;
				parse_data.aspect		= camera_data.ymag != 0.f ? camera_data.xmag / camera_data.ymag : 0.f;
				parse_data.z_far		= camera_data.zfar;	   // required in glTF for orthographic
			}
		}
	}
}	 // namespace age::external::cgltf::detail

// skeletons
namespace age::external::cgltf::detail
{
	struct skeleton_joint_data
	{
		uint32 joint_idx;			// == position in the vec
		uint32 node_idx;
		uint32 parent_joint_idx;	// invalid = root
		int	   parent_node_idx;		// node this joint's local is relative to. parent joint node, or the owner node for root joints. -1 = owner node is the scene root
	};

	// res[joint_idx] = joint. every parent_joint_idx < joint_idx
	// res[0].parent_node_idx == owner node_idx, if -1, scene_root is the owner
	dynamic_array<skeleton_joint_data>
	gen_skeleton_joint_arr(const gltf_data& data, std::span<const uint32> joint_node_idx_span) noexcept
	{
		if (joint_node_idx_span.empty()) { return {}; }

		// node idx -> skin_joint_idx , invalid when not a joint of this skin
		auto node_to_skin_joint = age::dynamic_array<uint32>::gen_sized_default(data.node_count);
		std::ranges::fill(node_to_skin_joint, age::get_invalid_id<uint32>());
		for (auto&& [skin_joint_idx, node_idx] : joint_node_idx_span | views::enumerate<uint32>)
		{
			node_to_skin_joint[node_idx] = skin_joint_idx;
		}

		auto parent_node_idx_arr	   = age::dynamic_array<int>::gen_sized_default(data.node_count);
		auto children_node_idx_vec_arr = age::dynamic_array<age::vector<uint32>>::gen_sized_default(data.node_count);
		auto root_node_idx_vec		   = age::vector<uint32>{};
		for (auto node_idx : joint_node_idx_span)
		{
			// nearest joint ancestor, non-joint nodes skipped. -1 = root.
			// skipping above the root joint is normal. skipping between two joints
			// is reported as non_joint_node_in_tree by fill_skeletons, via parent_node_idx != node.parent_idx
			auto parent_node_idx = data.p_node[node_idx].parent_idx;
			while (parent_node_idx >= 0 and node_to_skin_joint[parent_node_idx] == age::get_invalid_id<uint32>())
			{
				parent_node_idx = data.p_node[parent_node_idx].parent_idx;
			}

			parent_node_idx_arr[node_idx] = parent_node_idx;
			if (parent_node_idx < 0)
			{
				root_node_idx_vec.emplace_back(node_idx);
			}
			else
			{
				children_node_idx_vec_arr[parent_node_idx].emplace_back(node_idx);
			}
		}

		c_auto preorder_joint_node_idx_arr = [&] {
			auto buf = age::dynamic_array<uint32>::gen_sized_default(joint_node_idx_span.size());
			auto i	 = 0u;
			util::for_each_preorder(root_node_idx_vec, [&](uint32 node_idx) -> auto& { return children_node_idx_vec_arr[node_idx]; }, [&](uint32 node_idx) { buf[i++] = node_idx; });
			return buf;
		}();

		auto node_to_joint = age::dynamic_array<uint32>::gen_sized_default(data.node_count);
		std::ranges::fill(node_to_joint, age::get_invalid_id<uint32>());
		for (auto&& [joint_idx, node_idx] : preorder_joint_node_idx_arr | views::enumerate<uint32>)
		{
			node_to_joint[node_idx] = joint_idx;
		}

		c_auto calc_node_depth = [&](int32 node_idx) {
			uint32 depth = 0u;

			for (; node_idx >= 0; ++depth)
			{
				node_idx = data.p_node[node_idx].parent_idx;
			}

			return depth;
		};

		c_auto calc_node_lowest_common_ancestor = [&](int32 node_idx_l, int32 node_idx_r) {
			auto depth_l = calc_node_depth(node_idx_l);
			auto depth_r = calc_node_depth(node_idx_r);
			for (; depth_l < depth_r; --depth_r)
			{
				node_idx_r = data.p_node[node_idx_r].parent_idx;
			}
			for (; depth_r < depth_l; --depth_l)
			{
				node_idx_l = data.p_node[node_idx_l].parent_idx;
			}

			// now depth_l == depth_r

			for (; node_idx_l != node_idx_r;)
			{
				node_idx_l = data.p_node[node_idx_l].parent_idx;
				node_idx_r = data.p_node[node_idx_r].parent_idx;
			}

			// nod node_idx_l == node_idx_r

			return node_idx_l;
		};


		auto skeleton_owner_node_idx = data.p_node[root_node_idx_vec[0]].parent_idx;
		for (c_auto root_node_idx : root_node_idx_vec | std::views::drop(1))
		{
			skeleton_owner_node_idx = calc_node_lowest_common_ancestor(skeleton_owner_node_idx, data.p_node[root_node_idx].parent_idx);
		}

		auto res = age::dynamic_array<skeleton_joint_data>::gen_sized_default(preorder_joint_node_idx_arr.size());
		for (auto&& [joint_idx, node_idx] : preorder_joint_node_idx_arr | views::enumerate<uint32>)
		{
			c_auto parent_node_idx = parent_node_idx_arr[node_idx];

			res[joint_idx].joint_idx		= joint_idx;
			res[joint_idx].node_idx			= node_idx;
			res[joint_idx].parent_joint_idx = parent_node_idx >= 0 ? node_to_joint[parent_node_idx] : age::get_invalid_id<uint32>();
			res[joint_idx].parent_node_idx	= parent_node_idx >= 0 ? parent_node_idx : skeleton_owner_node_idx;
		}


		return res;
	}

	// [ skin_to_skeleton_lut, skeleton_joint_data_arr[skeleton_count][joint_count] ]
	// 1. merge multiple gltf skins into one skeleton if they share a joint (node)
	// 2. merge skeletons into one if they have the same owner node
	//    (owner = lowest common ancestor of the root joints' parents). not merged when the owner is the scene root (-1)
	std::tuple<age::dynamic_array<uint32>, age::dynamic_array<age::dynamic_array<skeleton_joint_data>>>
	gen_skeleton_scratch_data_arr(const gltf_data& data) noexcept
	{
		auto res = std::tuple<age::dynamic_array<uint32>, age::dynamic_array<age::dynamic_array<skeleton_joint_data>>>{};

		auto&& [skin_to_skeleton_lut, skeleton_joint_data_arr] = res;

		auto   node_to_skin_lut	  = age::dynamic_array<uint32>::gen_sized_copy(data.node_count, age::get_invalid_idx<uint32>());
		auto   skin_parent_arr	  = age::dynamic_array<uint32>::gen_sized_default(data.skin_count);
		c_auto find_root_skin_idx = [&](auto skin_idx) {
			while (skin_parent_arr[skin_idx] != skin_idx)
			{
				skin_idx = skin_parent_arr[skin_idx];
			}
			return skin_idx;
		};

		for (auto&& [skin_idx, skin] : std::span(data.p_skin, data.skin_count) | views::enumerate<uint32>)
		{
			skin_parent_arr[skin_idx]  = skin_idx;
			auto current_root_skin_idx = skin_idx;

			for (c_auto joint_node_idx : std::span(skin.p_joint_node_idx, skin.joint_count))
			{
				c_auto this_node_skin_idx = node_to_skin_lut[joint_node_idx];
				if (runtime::is_invalid_idx(this_node_skin_idx))
				{
					node_to_skin_lut[joint_node_idx] = skin_idx;
				}
				else
				{
					c_auto this_root_skin_idx = find_root_skin_idx(this_node_skin_idx);
					if (current_root_skin_idx != this_root_skin_idx)
					{
						skin_parent_arr[current_root_skin_idx] = this_root_skin_idx;
						current_root_skin_idx				   = this_root_skin_idx;
					}
				}
			}

			skin_parent_arr[skin_idx] = current_root_skin_idx;
		}

		skin_to_skeleton_lut			 = age::dynamic_array<uint32>::gen_sized_default(data.skin_count);
		auto skeleton_joint_node_idx_vec = age::vector<age::vector<uint32>>::gen_reserved(data.skin_count);

		for (auto skin_idx : views::loop(data.skin_count))
		{
			c_auto root = find_root_skin_idx(skin_idx);
			if (root == skin_idx)
			{
				skin_to_skeleton_lut[skin_idx] = skeleton_joint_node_idx_vec.size<uint32>();
				skeleton_joint_node_idx_vec.emplace_back(age::vector<uint32>{});
			}
		}
		for (c_auto skin_idx : views::loop(data.skin_count))
		{
			skin_to_skeleton_lut[skin_idx] = skin_to_skeleton_lut[find_root_skin_idx(skin_idx)];
		}

		for (auto&& [skin_idx, skin] : std::span(data.p_skin, data.skin_count) | views::enumerate<uint32>)
		{
			auto& joint_node_idx_vec = skeleton_joint_node_idx_vec[skin_to_skeleton_lut[skin_idx]];
			joint_node_idx_vec.reserve(joint_node_idx_vec.size() + skin.joint_count);

			for (c_auto joint_node_idx : std::span(skin.p_joint_node_idx, skin.joint_count))
			{
				if (node_to_skin_lut[joint_node_idx] == skin_idx)
				{
					joint_node_idx_vec.emplace_back(joint_node_idx);
				}
			}
		}

		auto skeleton_joint_data_vec = age::vector<age::vector<skeleton_joint_data>>::gen_sized(skeleton_joint_node_idx_vec.size<uint32>());

		for (auto&& [joint_data_arr, joint_node_idx_vec] : std::views::zip(skeleton_joint_data_vec, skeleton_joint_node_idx_vec))
		{
			joint_data_arr = gen_skeleton_joint_arr(data, joint_node_idx_vec);
		}

		// from now on, if skeleton_joint_node_idx_vec[skeleton_idx] is_empty means it is merged
		auto skeleton_unmerged_to_merged_lut = views::loop(skeleton_joint_data_vec.size<uint32>()) | std::ranges::to<age::dynamic_array<uint32>>();
		auto skeleton_count					 = 0u;
		for (auto&& [skeleton_idx_l, joint_data_vec_l, joint_node_idx_vec_l] : std::views::zip(views::loop(skeleton_joint_data_vec.size<uint32>()), skeleton_joint_data_vec, skeleton_joint_node_idx_vec))
		{
			if (joint_node_idx_vec_l.is_empty()) { continue; }

			++skeleton_count;
			c_auto skeleton_owner_l = joint_data_vec_l[0].parent_node_idx;

			if (skeleton_owner_l < 0) { continue; }

			for (auto&& [skeleton_idx_r, joint_data_vec_r, joint_node_idx_vec_r] : std::views::zip(views::loop(skeleton_joint_data_vec.size<uint32>()), skeleton_joint_data_vec, skeleton_joint_node_idx_vec) | std::views::drop(skeleton_idx_l + 1))
			{
				if (joint_node_idx_vec_r.is_empty()) { continue; }

				c_auto skeleton_owner_r = joint_data_vec_r[0].parent_node_idx;

				if (skeleton_owner_l != skeleton_owner_r) { continue; }

				// skeleton_owner_l == skeleton_owner_r and none of them are -1 (owner is scene_root, each will be instantiated under separate entities)

				joint_node_idx_vec_l.append_range(joint_node_idx_vec_r);
				joint_node_idx_vec_r.clear();
				skeleton_unmerged_to_merged_lut[skeleton_idx_r] = skeleton_idx_l;
			}
		}

		// skeleton_idx_umerged -> skeleton_idx (real)
		// skeleton_idx_remap[unmerged_skeleton_idx] == real (compacked) skeleton_idx
		auto skeleton_idx_remap = age::dynamic_array<uint32>::gen_sized_default(skeleton_unmerged_to_merged_lut.size<uint32>());

		for (auto skeleton_idx = 0u;
			 auto&& [unmerged_idx, merged_idx, skeleton_idx_res] : std::views::zip(views::loop(skeleton_unmerged_to_merged_lut.size<uint32>()), skeleton_unmerged_to_merged_lut, skeleton_idx_remap))
		{
			if (unmerged_idx != merged_idx)
			{
				skeleton_idx_res = skeleton_idx_remap[merged_idx];
			}
			else
			{
				skeleton_idx_res = skeleton_idx++;
			}
		}

		for (c_auto skin_idx : views::loop(data.skin_count))
		{
			skin_to_skeleton_lut[skin_idx] = skeleton_idx_remap[skin_to_skeleton_lut[skin_idx]];
		}

		skeleton_joint_data_arr = age::dynamic_array<age::dynamic_array<skeleton_joint_data>>::gen_sized_default(skeleton_count);
		for (auto	skeleton_idx = 0u;
			 auto&& joint_node_idx_vec : skeleton_joint_node_idx_vec)
		{
			if (joint_node_idx_vec.is_empty()) { continue; }

			skeleton_joint_data_arr[skeleton_idx++] = gen_skeleton_joint_arr(data, joint_node_idx_vec);
		}

		// returns [skin_to_skeleton_lut, skeleton_joint_data_arr]
		return res;
	}

	std::string
	make_joint_name(const gltf_data& data, int node_idx)
	{
		c_auto& node = data.p_node[node_idx];
		return node.name.count > 0 ? make_string(node.name) : "empty_joint_name__" + std::to_string(node_idx);
	}

	void
	fill_skeletons(const gltf_data& data, std::span<const uint32> skin_to_skeleton, std::span<const age::dynamic_array<skeleton_joint_data>> skeleton_joint_arr_span, asset::importer::gltf_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_skeleton_parse_warning_flags;
		using error	  = asset::importer::e::gltf_skeleton_parse_error_flags;

		res.gltf_skeleton_parse_data_vec.resize(skeleton_joint_arr_span.size());

		for (auto&& [skeleton_idx, joint_arr, parse_data] : std::views::zip(views::loop(skeleton_joint_arr_span.size()), skeleton_joint_arr_span, res.gltf_skeleton_parse_data_vec))
		{
			c_auto	skin_idx = *std::ranges::find_if(views::loop(data.skin_count), [&](auto s) { return skin_to_skeleton[s] == skeleton_idx; });
			c_auto& skin	 = data.p_skin[skin_idx];

			c_auto owner_node_idx = joint_arr[0].parent_node_idx;
			parse_data.name		  = owner_node_idx >= 0 and data.p_node[owner_node_idx].name.count > 0
									  ? make_string(data.p_node[owner_node_idx].name)
								  : skin.skeleton_root_node_idx >= 0 and data.p_node[skin.skeleton_root_node_idx].name.count > 0
									  ? make_string(data.p_node[skin.skeleton_root_node_idx].name)
									  : make_string(skin.name);

			parse_data.joint_vec	  = age::dynamic_array<asset::importer::gltf_skeleton_joint_parse_data>::gen_sized_default(joint_arr.size());
			parse_data.joint_name_vec = age::dynamic_array<std::string>::gen_sized_default(joint_arr.size());

			for (auto&& [joint_data, joint, joint_name] : std::views::zip(joint_arr, parse_data.joint_vec, parse_data.joint_name_vec))
			{
				c_auto& node = data.p_node[joint_data.node_idx];

				if (joint_data.parent_node_idx != node.parent_idx)
				{
					parse_data.warning_flags |= warning::non_joint_node_in_tree;
				}

				if (node.name.count <= 0)
				{
					parse_data.warning_flags |= warning::empty_joint_name;
				}

				joint.parent_idx = joint_data.parent_joint_idx;
				joint_name		 = make_joint_name(data, joint_data.node_idx);

				// joint local = world_to_parent * local_to_world. skipped nodes are absorbed
				// gltf : rh + y-up
				c_auto xm_world_rh = simd::load(float4x4(node.world_matrix));
				c_auto xm_local_rh = joint_data.parent_node_idx >= 0
									   ? simd::load(float4x4(data.p_node[joint_data.parent_node_idx].world_matrix)) | simd::mat_inv() | simd::mat_mul(xm_world_rh)
									   : xm_world_rh;
				c_auto xm_local_lh = xm_local_rh | simd::mat_mirror_z();

				auto&& [success, xm_translation, xm_quat, xm_scale] = xm_local_lh | simd::decompose_trs();

				if (success)
				{
					joint.translation = xm_translation | simd::to<float3>();
					joint.rotation	  = xm_quat | simd::to<float4>();
					joint.scale		  = xm_scale | simd::to<float3>();
				}
				else
				{
					c_auto local			  = xm_local_lh | simd::to<float4x4>();
					joint.translation		  = float3{ local[0][3], local[1][3], local[2][3] };
					joint.rotation			  = float4{ 0.f, 0.f, 0.f, 1.f };
					joint.scale				  = float3{ 1.f, 1.f, 1.f };
					parse_data.warning_flags |= warning::joint_decompose_failed;
				}
			}

			// joint names must be unique, binding is by name. sorted copy only, joint order stays preorder
			auto sorted_name_vec = age::dynamic_array<std::string_view>{ std::from_range, parse_data.joint_name_vec };
			std::ranges::sort(sorted_name_vec);
			if (std::ranges::adjacent_find(sorted_name_vec) != sorted_name_vec.end())
			{
				parse_data.error_flags |= error::duplicated_joint_name;
			}
		}
	}
}	 // namespace age::external::cgltf::detail

// meshes
namespace age::external::cgltf::detail
{
	// (uv1+, colors, custom) are not checked
	bool
	has_meshopt_attribute(const submesh_data& submsh_data) noexcept
	{
		c_auto is_meshopt = [](c_auto& attr) { return attr.view.p is_nullptr and attr.meshopt.block_idx >= 0; };
		if (is_meshopt(submsh_data.position) or is_meshopt(submsh_data.normal) or is_meshopt(submsh_data.tangent) or is_meshopt(submsh_data.index))
		{
			return true;
		}
		if (submsh_data.uv_count > 0 and is_meshopt(submsh_data.p_uv[0]))
		{
			return true;
		}

		c_auto joints_span	= std::span(submsh_data.p_joints, submsh_data.joints_count);
		c_auto weights_span = std::span(submsh_data.p_weights, submsh_data.joints_count);
		if (std::ranges::any_of(joints_span, is_meshopt) or std::ranges::any_of(weights_span, is_meshopt))
		{
			return true;
		}

		c_auto target_span = std::span(submsh_data.p_morph_target, submsh_data.morph_target_count);
		return std::ranges::any_of(target_span, [is_meshopt](c_auto& t) { return is_meshopt(t.position_delta) or is_meshopt(t.normal_delta) or is_meshopt(t.tangent_delta); });
	}

	void
	fill_submesh_mode(alpha_kind alpha, asset::importer::gltf_submesh_parse_data& res) noexcept
	{
		switch (alpha)
		{
		case alpha_kind::opaque:
		{
			res.raster_mode		   = graphics::e::mesh_raster_mode_kind::opaque;
			res.rt_alpha_test_mode = graphics::e::mesh_rt_alpha_test_mode_kind::opaque;
			res.rt_bake_mode	   = graphics::e::mesh_rt_bake_mode_kind::opaque;
			break;
		}
		case alpha_kind::mask:
		{
			res.raster_mode		   = graphics::e::mesh_raster_mode_kind::mask;
			res.rt_alpha_test_mode = graphics::e::mesh_rt_alpha_test_mode_kind::mask;
			res.rt_bake_mode	   = graphics::e::mesh_rt_bake_mode_kind::transparent;
			break;
		}
		case alpha_kind::blend:
		{
			res.raster_mode		   = graphics::e::mesh_raster_mode_kind::transparent;
			res.rt_alpha_test_mode = graphics::e::mesh_rt_alpha_test_mode_kind::blend;
			res.rt_bake_mode	   = graphics::e::mesh_rt_bake_mode_kind::transparent;
			break;
		}
		}
	}

	// dense float3 stream -> array, glTF RH -> age LH (z mirrored).
	// absent view -> empty array
	age::dynamic_array<float3>
	gen_float3_array_lh(const float_view& view) noexcept
	{
		auto res = age::dynamic_array<float3>::gen_sized_default(view.count / 3);
		for (auto&& [i, v] : std::views::zip(views::loop(res.size()), res))
		{
			v = float3{ view.p[i * 3], view.p[i * 3 + 1], -view.p[i * 3 + 2] };
		}
		return res;
	}

	// POSITION / NORMAL / TANGENT / TEXCOORD_0 -> vertex_fat, z mirrored, tangent.w flipped.
	// missing normal / tangent / uv get defaults, vertex_kind records what was present.
	// position missing -> position_missing
	asset::importer::e::gltf_mesh_parse_error_flags
	fill_vertex_buffer(const submesh_data& submsh_data, asset::importer::gltf_submesh_parse_data& res) noexcept
	{
		using error = asset::importer::e::gltf_mesh_parse_error_flags;

		if (submsh_data.position.view.p is_nullptr)
		{
			return error::position_missing;
		}

		c_auto vertex_count = submsh_data.vertex_count;
		c_auto has_normal	= submsh_data.normal.view.p is_not_nullptr;
		c_auto has_tangent	= submsh_data.tangent.view.p is_not_nullptr;
		c_auto has_uv		= submsh_data.uv_count > 0 and submsh_data.p_uv[0].view.p is_not_nullptr;

		c_auto position_arr = gen_float3_array_lh(submsh_data.position.view);
		c_auto normal_arr	= has_normal ? gen_float3_array_lh(submsh_data.normal.view) : age::dynamic_array<float3>{};
		c_auto p_tangent	= submsh_data.tangent.view.p;	 // float4, w = bitangent sign
		c_auto p_uv			= has_uv ? submsh_data.p_uv[0].view.p : nullptr;

		res.vertex_buffer = age::dynamic_array<asset::vertex_fat>::gen_sized_default(vertex_count);
		for (auto&& [i, vertex] : std::views::zip(views::loop(vertex_count), res.vertex_buffer))
		{
			vertex.pos		 = position_arr[i];
			vertex.normal	 = has_normal ? normal_arr[i] : float3{ 0.f, 0.f, 1.f };
			vertex.tangent	 = has_tangent ? float4{ p_tangent[i * 4], p_tangent[i * 4 + 1], -p_tangent[i * 4 + 2], -p_tangent[i * 4 + 3] } : float4{ 1.f, 0.f, 0.f, 1.f };
			vertex.uv_set[0] = has_uv ? float2{ p_uv[i * 2], p_uv[i * 2 + 1] } : float2::zero();
		}

		res.has_normal	= has_normal;
		res.has_tangent = has_tangent;
		res.has_uv		= has_uv;

		return error::none;
	}

	// indices -> triangle list. absent = 0..n-1 (spec), strip / fan unrolled, tail not a multiple of 3 dropped.
	// winding kept: z mirror turns glTF CCW into CW, the D3D default front face.
	// value >= vertex_count -> vertex_idx_out_of_range, bake would read past the vertex buffer
	asset::importer::e::gltf_mesh_parse_error_flags
	fill_index_buffer(const submesh_data& submsh_data, asset::importer::gltf_submesh_parse_data& res) noexcept
	{
		using error = asset::importer::e::gltf_mesh_parse_error_flags;

		c_auto vertex_count = res.vertex_buffer.size();

		// source indices, generated when the primitive is non-indexed
		auto src_arr = submsh_data.index.view.p is_not_nullptr
						 ? age::dynamic_array<uint32>{ submsh_data.index.view.p, submsh_data.index.view.p + submsh_data.index.view.count }
						 : age::dynamic_array<uint32>{ std::from_range, views::loop(cast_to<uint32>(vertex_count)) };

		if (std::ranges::any_of(src_arr, [&](auto idx) { return idx >= vertex_count; }))
		{
			return error::vertex_idx_out_of_range;
		}

		c_auto triangle_index_count = src_arr.size() - src_arr.size() % 3;
		res.index_buffer			= triangle_index_count == src_arr.size()
										? std::move(src_arr)
										: age::dynamic_array<uint32>{ src_arr.begin(), src_arr.begin() + triangle_index_count };
		return error::none;
	}

	std::tuple<asset::importer::e::gltf_mesh_parse_warning_flags, asset::importer::e::gltf_mesh_parse_error_flags>
	fill_vertex_skin_buffer(const submesh_data& submsh_data, uint32 mesh_joint_count, asset::importer::gltf_submesh_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_mesh_parse_warning_flags;
		using error	  = asset::importer::e::gltf_mesh_parse_error_flags;

		auto warning_flags = warning::none;

		c_auto vertex_count = res.vertex_buffer.size();
		c_auto set_count	= submsh_data.joints_count;
		c_auto joints_span	= std::span(submsh_data.p_joints, set_count);	  // uint4 per vertex per set
		c_auto weights_span = std::span(submsh_data.p_weights, set_count);	  // float4 per vertex per set

		if (set_count > 1)
		{
			warning_flags |= warning::joints_over_4_merged;
		}

		// gather buffers, 4 influences per set, reused per vertex
		c_auto influence_count = set_count * 4;
		auto   joint_arr	   = age::dynamic_array<uint32>::gen_sized_default(influence_count);
		auto   weight_arr	   = age::dynamic_array<float>::gen_sized_default(influence_count);

		res.vertex_skin_buffer = age::dynamic_array<asset::importer::vertex_skin_data>::gen_sized_default(vertex_count);
		for (auto&& [v, skin] : std::views::zip(views::loop(vertex_count), res.vertex_skin_buffer))
		{
			for (auto&& [s, joints, weights] : std::views::zip(views::loop(set_count), joints_span, weights_span))
			{
				for (auto k : views::loop(4))
				{
					c_auto joint_idx = joints.view.p[v * 4 + k];
					if (joint_idx >= mesh_joint_count)
					{
						return { warning_flags, error::joint_idx_out_of_range };
					}
					joint_arr[s * 4 + k]  = joint_idx;
					weight_arr[s * 4 + k] = weights.view.p[v * 4 + k];
				}
			}

			// top 4 by weight, normalized to sum 1. all zero stays all zero = rigid
			skin = [&] {
				if (influence_count > 4)
				{
					auto influence_view = std::views::zip(weight_arr, joint_arr);
					std::ranges::partial_sort(influence_view, influence_view.begin() + 4, std::ranges::greater{}, [](c_auto& t) { return std::get<0>(t); });
				}

				auto   res = asset::importer::vertex_skin_data{};
				c_auto sum = weight_arr[0] + weight_arr[1] + weight_arr[2] + weight_arr[3];
				if (sum <= 0.f)
				{
					return res;
				}
				for (auto a : views::loop(4))
				{
					res.joint_idx[a] = joint_arr[a];
					res.weight[a]	 = weight_arr[a] / sum;
				}
				return res;
			}();

			if (skin.weight == float4::zero())
			{
				warning_flags |= warning::vertex_skin_zero_weight;
			}
		}

		return { warning_flags, error::none };
	}

	asset::importer::e::gltf_mesh_parse_warning_flags
	fill_blend_shape_vec(const submesh_data& submsh_data, asset::importer::gltf_submesh_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_mesh_parse_warning_flags;

		auto warning_flags = warning::none;

		c_auto target_span	= std::span(submsh_data.p_morph_target, submsh_data.morph_target_count);
		res.blend_shape_vec = age::dynamic_array<asset::importer::blend_shape_parse_data>::gen_sized_default(submsh_data.morph_target_count);

		for (auto&& [target, blend_shape] : std::views::zip(target_span, res.blend_shape_vec))
		{
			blend_shape.position_delta_buffer = gen_float3_array_lh(target.position_delta.view);
			blend_shape.normal_delta_buffer	  = gen_float3_array_lh(target.normal_delta.view);
			blend_shape.tangent_delta_buffer  = gen_float3_array_lh(target.tangent_delta.view);

			if (target.uv_delta_count > 0 or target.color_delta_count > 0 or target.custom_count > 0)
			{
				warning_flags |= warning::blend_shape_uv_dropped;
			}
		}
		return warning_flags;
	}

	std::tuple<asset::importer::e::gltf_mesh_parse_warning_flags, asset::importer::e::gltf_mesh_parse_error_flags>
	fill_submesh(const gltf_data& data, const submesh_data& submsh_data, uint32 mesh_joint_count, asset::importer::gltf_submesh_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_mesh_parse_warning_flags;
		using error	  = asset::importer::e::gltf_mesh_parse_error_flags;

		auto warning_flags = asset::importer::e::gltf_mesh_parse_warning_flags::none;
		auto error_flags   = asset::importer::e::gltf_mesh_parse_error_flags::none;

		// todo
		if (submsh_data.mode != primitive_mode::triangles)
		{
			error_flags |= error::unsupported_topology;
			return { warning_flags, error_flags };
		}
		if (submsh_data.p_draco)
		{
			error_flags |= error::draco_compressed;
			return { warning_flags, error_flags };
		}
		if (has_meshopt_attribute(submsh_data))
		{
			error_flags |= error::meshopt_compressed;
			return { warning_flags, error_flags };
		}

		fill_submesh_mode(submsh_data.material_idx >= 0 ? data.p_material[submsh_data.material_idx].alpha_mode : alpha_kind::opaque, res);

		error_flags |= fill_vertex_buffer(submsh_data, res);
		if (error_flags != error::none)
		{
			return { warning_flags, error_flags };
		}
		error_flags |= fill_index_buffer(submsh_data, res);
		if (error_flags != error::none)
		{
			return { warning_flags, error_flags };
		}

		if (submsh_data.uv_count > 1)
		{
			warning_flags |= warning::uv1_dropped;
		}
		if (submsh_data.color_count > 0)
		{
			warning_flags |= warning::color_dropped;
		}
		if (submsh_data.custom_count > 0)
		{
			warning_flags |= warning::custom_attribute_dropped;
		}

		// mesh_joint_count == 0 means nothing binds a skin to this mesh, JOINTS are ignored (skin_without_skeleton, set by fill_meshes)
		if (mesh_joint_count > 0 and submsh_data.joints_count > 0)
		{
			c_auto[skin_warning_flags, skin_error_flags]  = fill_vertex_skin_buffer(submsh_data, mesh_joint_count, res);
			warning_flags								 |= skin_warning_flags;
			error_flags									 |= skin_error_flags;
			if (error_flags != error::none)
			{
				return { warning_flags, error_flags };
			}
		}
		warning_flags |= fill_blend_shape_vec(submsh_data, res);

		return { warning_flags, error_flags };
	}

	// fill per mesh blend shape data. every submesh matches (checked by fill_meshes).
	// names   : extras.targetNames or "blend_shape_N",
	// weights : mesh.weights or 0
	void
	fill_mesh_blend_shape(const mesh_data& msh_data, asset::importer::gltf_mesh_baked_parse_data& res) noexcept
	{
		c_auto blend_shape_count = res.submesh_vec.is_empty() ? 0u : res.submesh_vec[0].blend_shape_vec.size();
		c_auto name_span		 = std::span(msh_data.p_morph_target_name, msh_data.morph_target_name_count);
		c_auto weight_span		 = std::span(msh_data.morph_weight.p, msh_data.morph_weight.count);

		res.blend_shape_name_vec   = age::dynamic_array<std::string>::gen_sized_default(blend_shape_count);
		res.blend_shape_weight_vec = age::dynamic_array<float>::gen_sized_default(blend_shape_count);

		for (auto&& [i, name, weight] : std::views::zip(views::loop(blend_shape_count), res.blend_shape_name_vec, res.blend_shape_weight_vec))
		{
			name   = i < name_span.size() and name_span[i].count > 0 ? make_string(name_span[i]) : "blend_shape_" + std::to_string(i);
			weight = i < weight_span.size() ? weight_span[i] : 0.f;
		}
	}

	// skin.joints[slot] node names + skin.inverseBindMatrices[slot] as mesh_to_joint
	// accessor absent -> identity (spec).
	// meshopt-backed -> error::meshopt_compressed
	asset::importer::e::gltf_mesh_parse_error_flags
	fill_joint_binding(const gltf_data& data, const skin_data& skin, asset::importer::gltf_mesh_baked_parse_data& res) noexcept
	{
		using error	  = asset::importer::e::gltf_mesh_parse_error_flags;
		using warning = asset::importer::e::gltf_mesh_parse_warning_flags;

		c_auto& mesh_to_joint_view = skin.inverse_bind_matrix;
		if (mesh_to_joint_view.view.p is_nullptr and mesh_to_joint_view.meshopt.block_idx >= 0)
		{
			return error::meshopt_compressed;
		}

		c_auto joint_node_idx_span = std::span(skin.p_joint_node_idx, skin.joint_count);
		c_auto has_matrix		   = mesh_to_joint_view.view.p is_not_nullptr;	  // count == joint_count * 16, validated by cgltf

		res.joint_name_vec	  = age::dynamic_array<std::string>::gen_sized_default(skin.joint_count);
		res.mesh_to_joint_vec = age::dynamic_array<float4x4>::gen_sized_default(skin.joint_count);

		for (auto&& [skin_joint_idx, node_idx, joint_name, mesh_to_joint] : std::views::zip(views::loop(skin.joint_count), joint_node_idx_span, res.joint_name_vec, res.mesh_to_joint_vec))
		{
			if (data.p_node[node_idx].name.count == 0)
			{
				res.warning_flags |= warning::empty_joint_name;
			}

			joint_name = make_joint_name(data, node_idx);

			if (has_matrix)
			{
				c_auto& m_rh  = reinterpret_cast<const float (&)[16]>(mesh_to_joint_view.view.p[skin_joint_idx * 16]);
				mesh_to_joint = simd::load(float4x4(m_rh)) | simd::mat_mirror_z() | simd::to<float4x4>();
			}
			else
			{
				mesh_to_joint = float4x4::identity();
			}
		}
		return error::none;
	}

	// skin side data of a mesh (joint_name_vec, mesh_to_joint_vec) comes from the first skin any node binds it with.
	// a glTF primitive has exactly one material (KHR_materials_variants is dropped), so submesh bake options are fixed here
	void
	fill_meshes(const gltf_data& data, asset::importer::gltf_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_mesh_parse_warning_flags;
		using error	  = asset::importer::e::gltf_mesh_parse_error_flags;

		res.gltf_mesh_parse_data_vec.resize(data.mesh_count);

		// mesh_skin_idx_arr[mesh_idx] == skin_idx
		auto mesh_skin_idx_arr = age::dynamic_array<int32>::gen_sized_copy(data.mesh_count, -1);
		for (c_auto& node : std::span(data.p_node, data.node_count))
		{
			if (node.mesh_idx < 0 or node.skin_idx < 0) { continue; }

			c_auto is_same_skin = [](c_auto& data, int32 skin_idx_l, int32 skin_idx_r) {
				c_auto& skin_l = data.p_skin[skin_idx_l];
				c_auto& skin_r = data.p_skin[skin_idx_r];

				// if meshopt, fill_joint_binding will return anyway and span(nullptr, 0) is well-formed
				// if p == nullptr, count == zero
				return skin_l.joint_count == skin_r.joint_count
				   and std::ranges::equal(std::span(skin_l.p_joint_node_idx, skin_l.joint_count), std::span(skin_r.p_joint_node_idx, skin_r.joint_count))
				   and std::ranges::equal(std::span(skin_l.inverse_bind_matrix.view.p, skin_l.inverse_bind_matrix.view.count), std::span(skin_r.inverse_bind_matrix.view.p, skin_r.inverse_bind_matrix.view.count));
			};

			auto& first_skin_idx = mesh_skin_idx_arr[node.mesh_idx];
			if (first_skin_idx < 0)
			{
				first_skin_idx = node.skin_idx;
			}
			else if (first_skin_idx != node.skin_idx and is_same_skin(data, first_skin_idx, node.skin_idx) is_false)
			{
				res.gltf_mesh_parse_data_vec[node.mesh_idx].warning_flags |= warning::skin_dropped_by_multiple_skins;
			}
		}

		for (auto&& [mesh_idx, msh_data, parse_data] : std::views::zip(views::loop(data.mesh_count), std::span(data.p_mesh, data.mesh_count), res.gltf_mesh_parse_data_vec))
		{
			parse_data.name = make_string(msh_data.name);

			c_auto submesh_span = std::span(msh_data.p_submesh, msh_data.submesh_count);

			auto joint_count = 0;
			if (std::ranges::any_of(submesh_span, [](c_auto& s) { return s.joints_count > 0; }))
			{
				c_auto skin_idx = mesh_skin_idx_arr[mesh_idx];
				if (skin_idx == -1)
				{
					parse_data.warning_flags |= warning::skin_without_skeleton;	   // JOINTS but nothing binds a skin, the mesh stays static
				}
				else
				{
					c_auto& skin			= data.p_skin[skin_idx];
					parse_data.error_flags |= fill_joint_binding(data, skin, parse_data);
					joint_count				= skin.joint_count;
				}
			}

			parse_data.submesh_vec = age::dynamic_array<asset::importer::gltf_submesh_parse_data>::gen_sized_default(msh_data.submesh_count);
			for (auto&& [submsh_data, submsh_parse] : std::views::zip(submesh_span, parse_data.submesh_vec))
			{
				c_auto[warning_flags, error_flags]	= fill_submesh(data, submsh_data, joint_count, submsh_parse);
				parse_data.warning_flags		   |= warning_flags;
				parse_data.error_flags			   |= error_flags;
				if (error_flags != error::none)
				{
					break;
				}
			}

			// spec: every primitive of a mesh has the same morph target count in the same order
			c_auto blend_shape_count = parse_data.submesh_vec.is_empty() ? 0u : parse_data.submesh_vec[0].blend_shape_vec.size();
			if (std::ranges::any_of(parse_data.submesh_vec, [&](c_auto& s) { return s.blend_shape_vec.size() != blend_shape_count; }))
			{
				parse_data.error_flags |= error::blend_shape_count_mismatch;
			}

			if (parse_data.error_flags != error::none)
			{
				continue;	 // refused by import
			}

			fill_mesh_blend_shape(msh_data, parse_data);

			// unskinned submeshes of a skinned mesh stay rigid: zero weights
			c_auto has_skin = [](c_auto& s) { return s.vertex_skin_buffer.is_empty() is_false; };
			if (joint_count > 0 and std::ranges::all_of(parse_data.submesh_vec, has_skin) is_false)
			{
				for (auto& submesh : parse_data.submesh_vec)
				{
					if (submesh.vertex_skin_buffer.is_empty())
					{
						submesh.vertex_skin_buffer = age::dynamic_array<asset::importer::vertex_skin_data>::gen_sized_copy(submesh.vertex_buffer.size(), asset::importer::vertex_skin_data{ uint16_4::zero(), float4::zero() });
					}
				}
				parse_data.warning_flags |= warning::unskinned_submesh_rigid;
			}
		}
	}
}	 // namespace age::external::cgltf::detail

// model
namespace age::external::cgltf::detail
{
	// one model per mesh. model_idx == mesh_idx
	// this is because material_variants_dropped will be dropped
	void
	fill_models(const gltf_data& data, std::span<uint32> node_to_model_lut, asset::importer::gltf_parse_data& res) noexcept
	{
		using warning = asset::importer::e::gltf_model_parse_warning_flags;

		auto& model_vec = res.gltf_model_parse_data_vec;
		model_vec.reserve(data.mesh_count);

		for (auto&& [mesh_idx, msh_data] : std::span(data.p_mesh, data.mesh_count) | views::enumerate<uint32>)
		{
			auto& modl_parse_data	 = model_vec.emplace_back();
			modl_parse_data.mesh_idx = mesh_idx;

			modl_parse_data.submesh_gltf_material_idx_vec = age::dynamic_array<uint32>::gen_sized_default(msh_data.submesh_count);
			for (auto&& [i, submsh_data] : std::span(msh_data.p_submesh, msh_data.submesh_count) | views::enumerate<uint32>)
			{
				modl_parse_data.submesh_gltf_material_idx_vec[i] = submsh_data.material_idx >= 0 ? submsh_data.material_idx : age::get_invalid_id<uint32>();
			}
			if (msh_data.material_set_count > 0)
			{
				modl_parse_data.warning_flags |= warning::material_variants_dropped;
			}
		}

		for (auto&& [node_idx, node] : std::span(data.p_node, data.node_count) | views::enumerate<uint32>)
		{
			node_to_model_lut[node_idx] = node.mesh_idx >= 0 ? cast_to<uint32>(node.mesh_idx) : age::get_invalid_idx<uint32>();
		}
	}
}	 // namespace age::external::cgltf::detail

// entities
namespace age::external::cgltf::detail
{
	void
	fill_entities(const gltf_data& data, std::span<const uint32> skin_to_skeleton_lut, std::span<const age::dynamic_array<skeleton_joint_data>> skeleton_joint_data_arr_span, std::span<const uint32> node_to_model_lut, asset::importer::gltf_parse_data& res) noexcept
	{
		using gltf_warning = asset::importer::e::gltf_parse_warning_flags;
		using ent_warning  = asset::importer::e::gltf_entity_parse_warning_flags;

		res.gltf_entity_parse_data_vec.reserve(data.node_count);
		res.gltf_scene_parse_data_vec.resize(data.scene_count);
		res.default_scene_idx = data.default_scene_idx >= 0 and data.default_scene_idx < data.scene_count ? cast_to<uint32>(data.default_scene_idx) : age::get_invalid_idx<uint32>();

		struct node_joint_data
		{
			uint32 skeleton_idx		  = age::get_invalid_idx<uint32>();
			uint32 skeleton_joint_idx = age::get_invalid_idx<uint32>();

			int32 skeleton_owner_node_idx = -1;
		};

		auto node_joint_data_arr = age::dynamic_array<node_joint_data>::gen_sized_copy(data.node_count, node_joint_data{});

		// if node_owner_skeleton_arr[node_idx] is_not_invalid, this node owns a skeleton
		auto node_owner_skeleton_arr = age::dynamic_array<uint32>::gen_sized_copy(data.node_count, age::get_invalid_idx<uint32>());

		for (auto&& [skeleton_idx, joint_data_arr] : skeleton_joint_data_arr_span | views::enumerate<uint32>)
		{
			AGE_ASSERT(joint_data_arr.is_not_empty());
			c_auto owner_node_idx = joint_data_arr[0].parent_node_idx;
			if (owner_node_idx >= 0)
			{
				node_owner_skeleton_arr[owner_node_idx] = skeleton_idx;
			}

			for (c_auto& joint_data : joint_data_arr)
			{
				AGE_ASSERT(runtime::is_invalid_idx(node_joint_data_arr[joint_data.node_idx].skeleton_idx), "node cannot be shared between skeletons, this should be resolved by gen_skeleton_scratch_data function");
				node_joint_data_arr[joint_data.node_idx] = { skeleton_idx, joint_data.joint_idx, owner_node_idx };
			}
		}

		auto node_to_entity_lut = age::dynamic_array<uint32>::gen_sized_default(data.node_count);
		auto node_visited		= age::dynamic_array<bool>::gen_sized_copy(data.node_count, false);
		// gltf's skeleton asset count == skeleton instance count
		// skeleton idx -> owner entity idx lut
		// cleared every scene, each scene will have separate skeleton owner entity (unlike gltf node)
		auto skeleton_owner_entity_idx_lut = age::dynamic_array<uint32>::gen_sized_default(skeleton_joint_data_arr_span.size());
		// entity_owner_skeleton_idx_vec[entity_idx] == entity skeleton owner's skeleton idx
		auto entity_owner_skeleton_idx_vec = age::vector<uint32>{};

		for (auto&& [gltf_scene, scene_parse] : std::views::zip(std::span(data.p_scene, data.scene_count), res.gltf_scene_parse_data_vec))
		{
			scene_parse.name		 = make_string(gltf_scene.name);
			scene_parse.entity_begin = res.gltf_entity_parse_data_vec.size<uint32>();
			std::ranges::fill(node_to_entity_lut, age::get_invalid_idx<uint32>());
			std::ranges::fill(skeleton_owner_entity_idx_lut, age::get_invalid_idx<uint32>());

			// pre-pass
			// fill basic entity informations
			// fill node_to_entity_lut, node_visited, skeleton_owner_entity_idx_lut, entity_owner_skeleton_idx_vec
			util::for_each_preorder(
				std::span(gltf_scene.p_root_node_idx, gltf_scene.root_node_count),
				[&](int32 node_idx) { auto& node = data.p_node[node_idx]; return std::span<const int32>(node.p_child_node_idx, node.child_count); },
				[&](int32 node_idx) {
					if (runtime::is_invalid_idx(node_to_entity_lut[node_idx]) is_false)
					{
						res.warning_flags |= gltf_warning::duplicated_entity_in_scene_dropped;
						return;
					}

					node_visited[node_idx] = true;

					c_auto entity_id			 = res.gltf_entity_parse_data_vec.size<uint32>();
					node_to_entity_lut[node_idx] = entity_id;

					c_auto& node		   = data.p_node[node_idx];
					auto&	ent_parse_data = res.gltf_entity_parse_data_vec.emplace_back();


					ent_parse_data.name = make_string(node.name);

					ent_parse_data.translation = float3(node.local.translation) * float3(1.f, 1.f, -1.f);
					ent_parse_data.rotation	   = float4(node.local.rotation) * float4(-1.f, -1.f, 1.f, 1.f);
					ent_parse_data.scale	   = node.local.scale;
					ent_parse_data.parent_idx  = node.parent_idx >= 0 ? node_to_entity_lut[node.parent_idx] : age::get_invalid_idx<uint32>();
					ent_parse_data.model_idx   = node_to_model_lut[node_idx];
					ent_parse_data.light_idx   = node.light_idx >= 0 ? cast_to<uint32>(node.light_idx) : age::get_invalid_idx<uint32>();
					ent_parse_data.camera_idx  = node.camera_idx >= 0 ? cast_to<uint32>(node.camera_idx) : age::get_invalid_idx<uint32>();

					if (node.morph_weight.count > 0)
					{
						ent_parse_data.blend_shape_weight_override_arr = age::dynamic_array{ std::from_range_t{}, std::span(node.morph_weight.p, node.morph_weight.count) };
					}

					if (node.instance_count > 0)
					{
						ent_parse_data.warning_flags |= ent_warning::instancing_dropped;
						ent_parse_data.model_idx	  = age::get_invalid_idx<uint32>();
					}
					if (node.local_decompose_trs_failed)
					{
						ent_parse_data.warning_flags |= ent_warning::decompose_trs_failed;
						ent_parse_data.rotation		  = float4{ 0, 0, 0, 1.f };
					}

					c_auto& joint_data = node_joint_data_arr[node_idx];

					ent_parse_data.skeleton_idx = node_owner_skeleton_arr[node_idx];
					if (runtime::is_invalid_idx(ent_parse_data.skeleton_idx) is_false)
					{
						// this entity is the owner of this skeleton
						skeleton_owner_entity_idx_lut[ent_parse_data.skeleton_idx] = entity_id;
					}

					ent_parse_data.is_skinned = false;
					if (node.skin_idx >= 0)
					{
						AGE_ASSERT(node.mesh_idx >= 0, "skin without mesh, cgltf_validate should have rejected this. needs checking");


						if (node.instance_count > 0)
						{
							AGE_ASSERT(has_all(ent_parse_data.warning_flags, ent_warning::instancing_dropped));
						}
						else
						{
							AGE_ASSERT(runtime::is_invalid_idx(ent_parse_data.model_idx) is_false, "skin without model should not happen unless instancing_dropped");
							AGE_ASSERT(runtime::is_invalid_idx(res.gltf_model_parse_data_vec[ent_parse_data.model_idx].mesh_idx) is_false, "model without mesh should not happen in gltf parsing");

							if (res.gltf_mesh_parse_data_vec[res.gltf_model_parse_data_vec[ent_parse_data.model_idx].mesh_idx].mesh_to_joint_vec.is_empty())
							{
								ent_parse_data.warning_flags |= ent_warning::skin_on_static_mesh;
							}
							else
							{
								ent_parse_data.is_skinned = true;
							}
						}
					}

					c_auto joint_skeleton_idx = joint_data.skeleton_idx;
					c_auto skin_skeleton_idx  = ent_parse_data.is_skinned ? skin_to_skeleton_lut[node.skin_idx] : age::get_invalid_idx<uint32>();

					ent_parse_data.skeleton_joint_idx = joint_data.skeleton_joint_idx;

					if (runtime::is_invalid_idx(joint_skeleton_idx) is_false
						and runtime::is_invalid_idx(skin_skeleton_idx) is_false
						and joint_skeleton_idx != skin_skeleton_idx)
					{
						ent_parse_data.skeleton_joint_idx  = age::get_invalid_idx<uint32>();
						ent_parse_data.warning_flags	  |= ent_warning::joint_attach_dropped_by_skin_mismatch;
					}

					entity_owner_skeleton_idx_vec.emplace_back(runtime::is_invalid_idx(skin_skeleton_idx) is_false ? skin_skeleton_idx : joint_skeleton_idx);
				});


			// post-pass
			// resolve skeleton owner entity. runs after the traversal because the owner node may be visited later
			// this is done after the traversal because skeleton owner entity may not be entity's parent or not created yet (scene root)
			// if skeleton owner entity is not created yet, it will be created here
			// gltf import phases will handle entity reordering
			for (c_auto entity_idx : views::loop(res.gltf_entity_parse_data_vec.size<uint32>()) | std::views::drop(scene_parse.entity_begin))
			{
				c_auto owner_skeleton_idx = entity_owner_skeleton_idx_vec[entity_idx];
				if (runtime::is_invalid_idx(owner_skeleton_idx)) { continue; }

				if (runtime::is_invalid_idx(skeleton_owner_entity_idx_lut[owner_skeleton_idx]))
				{
					if (skeleton_joint_data_arr_span[owner_skeleton_idx][0].parent_node_idx /*skeleton owner node*/ >= 0)
					{
						// owner node exists but not in this scene
						res.warning_flags |= gltf_warning::skeleton_owner_node_not_in_scene;
					}

					skeleton_owner_entity_idx_lut[owner_skeleton_idx] = res.gltf_entity_parse_data_vec.size<uint32>();
					entity_owner_skeleton_idx_vec.emplace_back(age::get_invalid_idx<uint32>());

					auto& owner_entity_parse_data		 = res.gltf_entity_parse_data_vec.emplace_back();
					owner_entity_parse_data.name		 = std::format("skeleton_{}_owner", res.gltf_skeleton_parse_data_vec[owner_skeleton_idx].name);
					owner_entity_parse_data.translation	 = float3::zero();
					owner_entity_parse_data.rotation	 = float4{ 0, 0, 0, 1.f };
					owner_entity_parse_data.scale		 = float3::one();
					owner_entity_parse_data.skeleton_idx = owner_skeleton_idx;

					res.warning_flags |= gltf_warning::skeleton_owner_entity_generated;
				}

				res.gltf_entity_parse_data_vec[entity_idx].skeleton_owner_entity_idx = skeleton_owner_entity_idx_lut[owner_skeleton_idx];
			}

			scene_parse.entity_count = res.gltf_entity_parse_data_vec.size<uint32>() - scene_parse.entity_begin;
		}


		if (data.scene_count > 0 and std::ranges::contains(node_visited, false))
		{
			res.warning_flags |= gltf_warning::entity_not_in_scene_dropped;
		}
	}
}	 // namespace age::external::cgltf::detail

namespace age::external::cgltf
{
	void
	load(std::string_view path, asset::importer::gltf_parse_data& res) noexcept
	{
		using namespace detail;

		res.src_full_path = std::string{ path };

		auto data = load_gltf(path.data());
		if (data.error != load_error::none)
		{
			switch (data.error)
			{
			case load_error::invalid_format:
				res.error = asset::importer::e::gltf_parse_error_kind::invalid_format;
				break;
			case load_error::buffer_not_found:
				res.error = asset::importer::e::gltf_parse_error_kind::buffer_not_found;
				break;
			case load_error::internal:
				res.error = asset::importer::e::gltf_parse_error_kind::internal;
				break;
			default:
				AGE_ASSERT(false, "invalid error_kind {}", std::to_underlying(data.error));
				res.error = asset::importer::e::gltf_parse_error_kind::internal;
				break;
			}

			release(data);
			return;
		}


		fill_textures(data, res);
		fill_materials(data, res);
		fill_lights(data, res);
		fill_cameras(data, res);
		{
			// [ skin_to_skeleton_lut, skeleton_joint_data_arr[skeleton_count][joint_count] ]
			c_auto && [ skin_to_skeleton_lut, skeleton_joint_data_arr ] = gen_skeleton_scratch_data_arr(data);

			fill_skeletons(data, skin_to_skeleton_lut, skeleton_joint_data_arr, res);
			fill_meshes(data, res);

			auto node_to_model_lut = age::dynamic_array<uint32>::gen_sized_copy(data.node_count, age::get_invalid_id<uint32>());
			fill_models(data, node_to_model_lut, res);
			fill_entities(data, skin_to_skeleton_lut, skeleton_joint_data_arr, node_to_model_lut, res);
		}


		if (data.animation_count > 0)
		{
			res.warning_flags |= asset::importer::e::gltf_parse_warning_flags::animation_dropped;
		}

		release(data);
	}
}	 // namespace age::external::cgltf