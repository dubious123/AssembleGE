#pragma once

namespace age::ecs
{
	template <typename t>
	consteval auto
	get_component_name()
	{
		return "unnamed component";
	}

	template <typename t, std::size_t i>
	consteval auto
	get_component_name_at()
	{
		return "unnamed component";
	}

	template <typename t>
	consteval bool
	is_ecs_component()
	{
		return false;
	}

	class entity_storage_tag { };

	class entity_block_tag { };

	class query_tag { };

	template <typename t_cmp>
	concept cx_component = requires {
		requires std::is_trivially_copyable_v<std::decay_t<t_cmp>>;
		requires std::is_trivially_destructible_v<std::decay_t<t_cmp>>;
		requires std::is_standard_layout_v<std::decay_t<t_cmp>>;
	} and ecs::is_ecs_component<std::decay_t<t_cmp>>();

	template <typename t>
	concept cx_entity_storage = requires { typename std::remove_cvref_t<t>::ecs_tag; }
							and std::is_same_v<typename std::remove_cvref_t<t>::ecs_tag, entity_storage_tag>;

	template <typename t>
	concept cx_entity_block = requires { typename std::remove_cvref_t<t>::ecs_tag; }
						  and std::is_same_v<typename std::remove_cvref_t<t>::ecs_tag, entity_block_tag>;

	template <typename t>
	concept cx_query = requires { typename std::remove_cvref_t<t>::ecs_tag; }
				   and std::is_same_v<typename std::remove_cvref_t<t>::ecs_tag, query_tag>;
}	 // namespace age::ecs

namespace age::ui::e
{
	// css object-fit
	AGE_DEFINE_ENUM(fit_mode_kind, uint8, contain, cover, fill, none, scale_down);

	AGE_DEFINE_ENUM(shape_kind, uint8,
					rect,
					circle,
					arrow_right,
					text,
					check,
					rounded_rect,
					triangle,
					cross,
					arc,
					pie,
					pie_range,
					mesh);
}	 // namespace age::ui::e

namespace age::graphics
{
	AGE_DEFINE_ENUM(color_space, uint8, srgb, hdr);
}	 // namespace age::graphics

namespace age::graphics::e
{
	AGE_DEFINE_ENUM(camera_kind, uint8, perspective, orthographic);

	AGE_DEFINE_ENUM_WITH_VALUE(light_kind, uint16,
							   (directional, 0),
							   (point, 1),
							   (spot, 2),
							   (area, 3),
							   (volumn, 4));

	AGE_DEFINE_ENUM_WITH_VALUE(texture_format, uint16,
							   (rgba8_unorm, 0),
							   (rgba8_unorm_srgb, 1),

							   (rgba16_float, 2),
							   (rgba16_unorm, 3),

							   (rgba32_float, 4),

							   (r8_unorm, 5),
							   (r8g8_unorm, 6),
							   (r16_float, 7),
							   (r16g16_float, 8),

							   (bc1_unorm, 9),	   // 4 bpp, RGB(+1bit alpha)
							   (bc1_unorm_srgb, 10),

							   (bc3_unorm, 11),	   // 8 bpp, RGBA legacy
							   (bc3_unorm_srgb, 12),

							   (bc4_unorm, 13),	   // 4 bpp, single channel - occlusion
							   (bc4_snorm, 14),

							   (bc5_unorm, 15),	   // 8 bpp, two channel (RG) - normal
							   (bc5_snorm, 16),

							   (bc6h_uf16, 17),	   // 8 bpp, RGB float - HDR
							   (bc6h_sf16, 18),

							   (bc7_unorm, 19),	   // 8 bpp, high quality LDR
							   (bc7_unorm_srgb, 20),
							   (r32_float, 21),
							   (r32g32_uint, 22),
							   (r16g16b16a16_float, 23),
							   (r8_uint, 24),
							   (d32_float, 25),
							   (d16_unorm, 26),
							   (r11g11b10_float, 27),
							   (r16g16_snorm, 28),
							   (rgba8_typeless, 29));

	// todo, rename rt_instance_flags
	AGE_DEFINE_ENUM_FLAGS(rt_mask_kind, uint8,
						  (none, 0x00),
						  (opaque, 0x01),
						  (transparent, 0x02),
						  (omm, 0x04),
						  (debug, 0x08),
						  (always_on_top, 0x80),
						  (all, 0xff));

	AGE_DEFINE_ENUM(mesh_rt_bake_mode_kind, uint8, opaque, transparent, omm_opaque, omm_transparent);
	AGE_DEFINE_ENUM(mesh_raster_mode_kind, uint8, opaque, transparent, mask);
	AGE_DEFINE_ENUM(mesh_rt_alpha_test_mode_kind, uint8, opaque, blend, mask);

	AGE_DEFINE_ENUM(mesh_raster_override_kind, uint8, none, force_opaque, force_transparent, force_mask);
	AGE_DEFINE_ENUM(mesh_rt_alpha_test_override_kind, uint8, none, opaque, blend, mask);

	AGE_DEFINE_ENUM_FLAGS(model_render_option_flags, uint8,
						  (none, uint8(0u << 0u)),
						  (fade, uint8(1u << 0u)),			 // opaque : dither, mask : dither on survived, transparent : alpha * fade
						  (disable_omm, uint8(1u << 1u)),	 // disable omm, use runtime alpha cutoff
						  (force_double_sided, uint8(1u << 2u)));

	AGE_DEFINE_ENUM(sampler_kind, uint8,
					linear_wrap,
					linear_clamp,
					linear_mirror,
					point_wrap,
					point_clamp,
					point_mirror);

	AGE_DEFINE_ENUM(material_shading_model_kind, uint8,
					pbr_default,
					pbr_unlit);

	AGE_DEFINE_ENUM_FLAGS(ddgi_debug_flags, uint32,
						  (none, 0),
						  (render_probe_in_hole, 0x1),
						  (render_irradiance, 0x2),
						  (render_visibility, 0x4),
						  (render_front_back, 0x8),
						  (render_level, 0x10),
						  (render_weight_sum, 0x20),
						  (render_ray_count, 0x40),
						  (render_state, 0x80),
						  (render_msme, (1u << 8u)),
						  (render_ray_factor, (1u << 9u)),
						  (render_probe, (1u << 31u)));

	AGE_DEFINE_ENUM_FLAGS(gibs_debug_flags, uint32,
						  (none, 0),
						  (freeze_spawn_kill, (1u << 0u)),
						  (render_tile, (1u << 1u)),
						  (render_cell, (1u << 2u)),

						  (render_tile_surfel_count, (1u << 3u)),
						  (render_cell_surfel_count, (1u << 4u)),

						  (render_tile_surfels, (1u << 6u)),
						  (render_cell_surfels, (1u << 7u)),
						  (render_id_hash, (1u << 9u)),
						  (render_radiance, (1u << 10u)),		 // di (tile_surfel : black)
						  (render_irradiance, (1u << 11u)),		 // gi
						  (render_normal, (1u << 12u)),

						  (render_visibility, (1u << 13u)),
						  (render_near_coverage, (1u << 14u)),
						  (render_far_coverage, (1u << 15u)),	 // tile_surfel : black

						  (render_ray_count, (1u << 16u)),
						  (render_age, (1u << 17u)));

	AGE_DEFINE_ENUM_FLAGS(gist_debug_flags, uint32,
						  (none, 0),
						  (freeze_surfel_spawn, (1u << 0u)),
						  (freeze_surfel_kill, (1u << 1u)),
						  (freeze_surfel_radius, (1u << 2u)),
						  (freeze_surfel_ray_trace, (1u << 3u)));


	AGE_DEFINE_ENUM_FLAGS(ao_debug_flags, uint32,
						  (none, 0),
						  (render_ao_buffer, (1u << 0u)));
}	 // namespace age::graphics::e

// debug view
namespace age::graphics::e
{
	AGE_DEFINE_ENUM(hrp_debug_view_system_kind, uint32, none, common, aa, ao, ddgi, gibs, gist);

	AGE_DEFINE_ENUM_FLAGS(hrp_debug_view_slot_option_flags, uint32,
						  (none, 0),
						  (enabled, (1u << 0u)),
						  (clear, (1u << 1u)),
						  (freeze, (1u << 2u)),
						  (enable_cursor_interact, (1u << 3u)),
						  (mark_nan, (1u << 4u)),
						  (mark_inf, (1u << 5u)),
						  (mark_zero, (1u << 6u)),
						  (mark_above_max, (1u << 7u)),
						  (mark_below_min, (1u << 8u)));

	AGE_DEFINE_ENUM(hrp_debug_view_color_map_kind, uint32, none, grayscale, turbo, viridis, plasma, magma, inferno);
	AGE_DEFINE_ENUM(hrp_debug_view_sys_common_popup_kind, uint32, none, zoom, value);
	AGE_DEFINE_ENUM(hrp_debug_view_kind_sys_common, uint32,
					none,
					depth,
					visibility,
					render_id,
					meshlet_id,
					prim_id,
					material_id,
					vertex_normal,
					shading_normal,
					base_color,
					occlusion,
					roughness,
					metallic,
					emissive,
					motion,
					rt_surface_gap, /*gap between px_world_pos, rt_world_pos*/
					mesh_surface_gap /*gap between px_world_pos, vertex_world_pos*/);

	AGE_DEFINE_ENUM_FLAGS(hrp_debug_view_overlay_flags_sys_common, uint32,
						  (none, 0),
						  (opaque, (1u << 0u)),
						  (transparent, (1u << 1u)),
						  (opaque_edge, (1u << 2u)),
						  (transparent_edge, (1u << 3u)));


	AGE_DEFINE_ENUM(hrp_debug_view_gist_popup_kind, uint32, none, zoom, value, cell_surfel_chebyshev);
	AGE_DEFINE_ENUM(hrp_debug_view_kind_gist, uint32,
					none,
					luminance_tile,
					tile,
					tile_ray_pdf,
					tile_ray_pdf_ratio,
					tile_ray_pdf_guided_ratio,
					tile_ray_distance,
					tile_ray_dir_local,
					tile_ray_dir_world,
					tile_ray_radiance,
					tile_ray_irradiance,
					tile_lumiance_sum,
					cell,
					cell_radiance,
					cell_irradiance,
					cell_irradiance_near,
					cell_irradiance_far,
					cell_near_conf,
					cell_near_conf_ratio,
					cell_far_conf,
					cell_far_conf_ratio,
					cell_near_surfel_count,
					cell_far_surfel_count,
					cell_near_far_surfel_count,
					cell_surfel_count_total,
					cell_surfel_id,
					cell_surfel_radiance,
					cell_surfel_irradiance,
					cell_surfel_visibility,
					cell_surfel_ray_count,
					cell_surfel_invalid_ray_count,
					cell_surfel_invalid_ray_ratio,
					cell_surfel_luminance_sum,
					cell_surfel_luminance_sum_ratio,
					cell_surfel_near_coverage,
					cell_surfel_far_coverage,
					cell_surfel_spawn_prob_near,
					cell_surfel_spawn_prob_far,
					cell_surfel_spawn_prob,
					cell_surfel_kill_prob_near,
					cell_surfel_kill_prob_far,
					cell_surfel_kill_prob,
					gi_diffuse,
					gi_diffuse_raw,
					gi_diffuse_age,
					gi_diffuse_moments,
					gi_diffuse_cv,
					gi_diffuse_variance,
					gi_diffuse_is_round_robin,
					gi_specular,
					gi_specular_raw,
					gi_specular_raw_radiance,
					gi_specular_age,
					gi_specular_hit_dist,
					gi_specular_curvature,
					gi_specular_motion,
					gi_specular_filter_radius,
					gi_specular_ray_pdf,
					gi_specular_ray_dir_local,
					gi_specular_ray_dir_world,
					adaptive_ray_count,
					adaptive_ray_type,
					adaptive_ray_type_is_new_born,
					adaptive_ray_type_is_specular,
					adaptive_ray_type_is_variance,
					stat_cell_surfel,
					stat_ray_count,
					stat_adaptive_ray_count_cap,
					stat_adaptive_ray_entry_prob);

	AGE_DEFINE_ENUM_FLAGS(hrp_debug_view_overlay_flags_gist, uint32,
						  (none, 0),
						  (diffuse_tile_grid, (1u << 0u)),
						  (diffuse_luminance_tile_grid, (1u << 1u)),
						  (cell_grid, (1u << 2u)));

	AGE_DEFINE_ENUM(hrp_debug_view_gist_cell_surfel_select_kind, uint32,
					none,
					max_contribution,
					max_near_contribution,
					max_far_contribution,
					oldest,
					closest);

	AGE_DEFINE_ENUM_FLAGS(hrp_debug_view_cursor_overlay_flags_gist, uint32,
						  (none, 0),
						  (cell_surfel_all, (1u << 0u)),
						  (cell_surfel_one, (1u << 1u)),
						  (cell_visibility, (1u << 2u)),
						  (cell_near_contribution, (1u << 3u)),
						  (cell_far_contribution, (1u << 4u)),
						  (cell_surfel_ray, (1u << 5u)),
						  (cell_irradiance, (1u << 6u)),
						  (cell_irradiance_near, (1u << 7u)),
						  (cell_irradiance_far, (1u << 8u)),
						  (adaptive_ray, (1u << 9u)),
						  (gi_diffuse_reconstruct_tap, (1u << 10u)),
						  (gi_diffuse_reproject_tap, (1u << 11u)),
						  (gi_specular_reconstruct_tap, (1u << 12u)),
						  (gi_specular_reproject_tap, (1u << 13u))

	);

	AGE_DEFINE_ENUM(hrp_debug_view_gist_cell_surfel_render_flags, uint32,
					none,
					// brush
					id,
					normal,
					radiance,
					irradiance,
					// color
					normal_ray,
					point,
					disk,
					sphere);


}	 // namespace age::graphics::e

namespace age::graphics
{
	using t_resource_id = uint32;

	struct resource_handle
	{
		t_resource_id id = age::get_invalid_id<t_resource_id>();

		FORCE_INLINE auto*
		operator->() noexcept;

		FORCE_INLINE c_auto*
		operator->() const noexcept;
	};
}	 // namespace age::graphics

namespace age::asset
{
	struct mesh_editable;


}	 // namespace age::asset

namespace age::runtime
{
	FORCE_INLINE bool
	is_handle_invalid(auto&& any_handle) noexcept
	{
		if constexpr (std::is_same_v<uint32, BARE_OF(any_handle.id)>)
		{
			return any_handle.id == invalid_id_uint32;
		}
		else
		{
			static_assert(false);
		}
	}

	FORCE_INLINE bool
	is_handle_valid(auto&& any_handle) noexcept
	{
		if constexpr (std::is_same_v<uint32, BARE_OF(any_handle.id)>)
		{
			return any_handle.id != invalid_id_uint32;
		}
		else
		{
			static_assert(false);
		}
	}
}	 // namespace age::runtime