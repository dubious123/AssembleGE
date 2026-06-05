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
							   (r11g11b10_float, 27));

	AGE_DEFINE_ENUM_WITH_VALUE(rt_mask_kind, uint8,
							   (opaque, 0x01),
							   (transparent, 0x02),
							   (mask, 0x04),
							   (debug, 0x08),
							   (always_on_top, 0x80),
							   (all, 0xff));

	AGE_ENUM_FLAG_OPERATORS(rt_mask_kind);

	AGE_DEFINE_ENUM_WITH_VALUE(ddgi_debug_flags, uint32,
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

	AGE_ENUM_FLAG_OPERATORS(ddgi_debug_flags);

	AGE_DEFINE_ENUM_WITH_VALUE(gibs_debug_flags, uint32,
							   (none, 0),
							   (render_surfels, (1u << 0u)),
							   (render_radiance, (1u << 1u)),
							   (render_irradiance, (1u << 2u)),
							   (render_visibility, (1u << 3u)),
							   (render_instability, (1u << 4u)),
							   (render_ray_count, (1u << 5u)),
							   (render_msme, (1u << 6u)),
							   (render_id_hash, (1u << 7u)),
							   (render_normal, (1u << 8u)),
							   (render_cell_occupancy, (1u << 9u)),
							   (render_show_irradiance_atlas, (1u << 10u)),
							   (render_show_visibility_atlas, (1u << 11u)),
							   (freeze_spawn, (1u << 12u)));

	AGE_ENUM_FLAG_OPERATORS(gibs_debug_flags);
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