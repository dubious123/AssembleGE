#include "hrp_common.asli"

[numthreads(16, 16, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	const debug_view_data data = debug_view::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(dispatch_thread_id.xy);

	if (any(px >= extent)) { return; }

	rw_texture_2d<float4> debug_view_buffer = global_resource_buffer[data.h_debug_view_buffer_uav_id];

	const debug_view_popup_data popup_data = debug_view::load_popup_data(data);

	const int32_4 popup_rect = popup_data.rect;
	if (contains(popup_rect, px))
	{
		if (is_border_unchecked(popup_rect, px, int32(data.popup_border_thickness)))
		{
			debug_view_buffer[px] = float4(0.f, 0.f, 0.f, 1.f);
			return;
		}

		const float2 uv_local = debug_view::calc_uv_local(popup_rect, px);

		const debug_view_cursor_data cursor_data = debug_view::load_cursor_data(data, popup_data.pick_count == 0u ? 0u : 1u);

		float3 color = color_black.rgb;

		switch (cursor_data.system_kind)
		{
		case AGE_DEBUG_VIEW_SYSTEM_KIND_COMMON:
		{
			color = debug_view::eval_popup_common(data, popup_data, cursor_data, uv_local);
			break;
		}
		case AGE_DEBUG_VIEW_SYSTEM_KIND_GIST:
		{
			color = gist_debug_view::eval_popup(data, popup_data, cursor_data, uv_local);
			break;
		}
		default:
			break;
		}

		debug_view_buffer[px] = float4(color, 1.f);
		return;
	}


	for (int32 slot_idx = int32(data.slot_count) - 1; slot_idx >= 0; --slot_idx)
	{
		const debug_view_slot_data slot_data = debug_view::load_slot_data(data, uint32(slot_idx));

		if (debug_view::slot::enabled(slot_data) is_false) { continue; }

		const int32_4 rect = debug_view::slot::load_rect(data, uint32(slot_idx));

		if (any(px < rect.xy) or any(px >= rect.zw)) { continue; }


		if (debug_view::slot::is_fullscreen(slot_data, uint32(slot_idx)) is_false and is_border_unchecked(rect, px, int32(slot_data.border_thickness)))
		{
			debug_view_buffer[px] = float4(0.f, 0.f, 0.f, 1.f);
			return;
		}

		const float2 uv_local		= debug_view::calc_uv_local(rect, px);
		float3		 color			= slot_data.background_color;
		float4		 overlay		= zero<float4>();
		float4		 cursor_overlay = zero<float4>();

		switch (slot_data.system_kind)
		{
		case AGE_DEBUG_VIEW_SYSTEM_KIND_COMMON:
		{
			color		   = debug_view::eval_base_common(data, slot_data, uv_local);
			overlay		   = debug_view::eval_overlay_common(data, slot_data, uv_local);
			cursor_overlay = debug_view::eval_cursor_overlay_common(data, slot_data, uv_local);
			break;
		}
		case AGE_DEBUG_VIEW_SYSTEM_KIND_GIST:
		{
			if (gist::enabled())
			{
				color		   = gist_debug_view::eval_base(data, slot_data, uv_local);
				overlay		   = gist_debug_view::eval_overlay(data, slot_data, uv_local);
				cursor_overlay = gist_debug_view::eval_cursor_overlay(data, slot_idx, slot_data, uv_local);
			}

			break;
		}
		default:
			break;
		}

		float3 color_res = lerp(color, overlay.rgb, overlay.a);
		color_res		 = lerp(color_res, cursor_overlay.rgb, cursor_overlay.a);

		debug_view_buffer[px] = float4(color_res, slot_data.alpha);
		return;
	}

	debug_view_buffer[px] = zero<float4>();
}