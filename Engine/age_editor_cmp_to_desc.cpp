#include "age_pch.hpp"
#include "age.hpp"

namespace age::editor
{
	age::graphics::render_pipeline::debug_view_desc
	cmp_to_desc(const age::ecs::debug_view_config& cmp) noexcept
	{
		auto debug_view_desc = graphics::render_pipeline::debug_view_desc{
			.popup_view_size_uv		= cmp.popup_view_size_uv,
			.popup_border_thickness = cmp.popup_border_thickness,
			.cursor_px				= cmp.cursor_px,
			.nan_color				= cmp.nan_color,
			.pos_inf_color			= cmp.pos_inf_color,
			.neg_inf_color			= cmp.neg_inf_color,
			.zero_color				= cmp.zero_color,
			.below_min_color		= cmp.below_min_color,
			.above_max_color		= cmp.above_max_color,
		};
		auto slot_desc_arr = age::array<graphics::render_pipeline::debug_view_slot_desc, 16>{};

		c_auto slot_config_to_desc = [](auto& desc, auto& config, bool is_fullscreen = false) {
			desc.system_kind							= config.system_kind;
			desc.system_debug_view_kind					= config.system_debug_view_kind;
			desc.system_debug_view_overlay_flags		= config.system_debug_view_overlay_flags;
			desc.system_debug_view_cursor_overlay_flags = config.system_debug_view_cursor_overlay_flags;
			desc.system_popup_view_kind					= config.system_popup_view_kind;
			desc.option_flags							= config.option_flags;
			desc.color_map_kind							= config.color_map_kind;
			desc.size_uv								= is_fullscreen ? float2::one() : config.size_uv;
			desc.offset_uv								= config.offset_uv;
			desc.pos_uv									= config.pos_uv;
			desc.scalar_range_min						= config.scalar_range_min;
			desc.scalar_range_max						= config.scalar_range_max;
			desc.alpha									= config.alpha;
			desc.popup_zoom								= config.popup_zoom;
			desc.background_color						= config.background_color;
			desc.border_thickness						= config.border_thickness;

			std::memcpy(desc.payload, config.payload, sizeof(config.payload));
		};

		slot_config_to_desc(debug_view_desc.fullscreen_slot_desc, cmp.fullscreen_slot_config, true);

		AGE_ASSERT(cmp.slot_count >= 1);

		for (auto&& [i, config] : cmp.slot_config_arr | std::views::take(cmp.slot_count - 1) | std::views::enumerate)
		{
			slot_config_to_desc(slot_desc_arr[i], config);
		}

		debug_view_desc.slot_descs = std::span{ slot_desc_arr.data(), cmp.slot_count - 1 };

		return debug_view_desc;
	}

	age::graphics::render_pipeline::model_render_option
	cmp_to_desc(const age::ecs::model_render_option& cmp) noexcept
	{
		return {
			.raster_override_kind		 = cmp.raster_override_kind,
			.rt_alpha_test_override_kind = cmp.rt_alpha_test_override_kind,
			.option_flags				 = cmp.option_flags,
			.fade_unorm8				 = cmp.fade_unorm8,
		};
	}
}	 // namespace age::editor