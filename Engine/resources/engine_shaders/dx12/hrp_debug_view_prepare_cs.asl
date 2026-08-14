#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs()

{
	const debug_view_data data	 = debug_view::load_data();
	const int32_2		  extent = int32_2(backbuffer_size);

	int32_2 curr_pos		= int32_2(extent.x, 0);	   // top_right, horizontal_inv
	int32	next_line_pos_y = 0;

	int32	hit_slot_id = -1;
	int32_4 hit_rect	= zero<int32_4>();

	for (uint32 i = 0u; i < data.slot_count; ++i)
	{
		const debug_view_slot_data slot_data = debug_view::load_slot_data(data, i);

		if (debug_view::slot::enabled(slot_data) is_false)
		{
			debug_view::set_slot_rect(data, zero<int32_4>(), i);
			continue;
		}

		const int32_2 size = int32_2(slot_data.size_uv * backbuffer_size);

		int32_2 pos;

		if (all(slot_data.pos_uv >= 0.f))
		{
			pos = int32_2(slot_data.pos_uv * backbuffer_size);
		}
		else
		{
			pos = int32_2(curr_pos.x - size.x, curr_pos.y);

			if (pos.x < 0)
			{
				pos = int32_2(extent.x - size.x, next_line_pos_y);
			}

			if (i != 0)
			{
				curr_pos		= int32_2(pos.x, pos.y);
				next_line_pos_y = max(next_line_pos_y, pos.y + size.y);
			}
		}


		pos = clamp(pos, zero<int32_2>(), max(extent - size, zero<int32_2>()));

		const int32_4 rect = int32_4(pos, pos + size);

		debug_view::set_slot_rect(data, rect, i);

		if (contains(rect, data.cursor_px))
		{
			hit_slot_id = int32(i);
			hit_rect	= rect;
		}
	}

	debug_view_cursor_data hover = zero<debug_view_cursor_data>();

	if (hit_slot_id >= 0)
	{
		const debug_view_slot_data slot_data = debug_view::load_slot_data(data, uint32(hit_slot_id));
		const float2			   uv_local	 = debug_view::calc_uv_local(hit_rect, data.cursor_px);

		hover.system_kind							 = slot_data.system_kind;
		hover.system_debug_view_kind				 = slot_data.system_debug_view_kind;
		hover.system_debug_view_overlay_flags		 = slot_data.system_debug_view_overlay_flags;
		hover.system_debug_view_cursor_overlay_flags = slot_data.system_debug_view_cursor_overlay_flags;
		hover.slot_id								 = uint32(hit_slot_id);
		hover.frame_index							 = frame_index;
		hover.px_local								 = data.cursor_px - hit_rect.xy;

		switch (slot_data.system_kind)
		{
		case AGE_DEBUG_VIEW_SYSTEM_KIND_COMMON:
			debug_view::eval_base_common(data, slot_data, uv_local, hover);
			debug_view::eval_overlay_common(data, slot_data, uv_local, hover);
			break;
		case AGE_DEBUG_VIEW_SYSTEM_KIND_GIST:
			// gist::debug_view::eval_base(data, slot_data, uv_local, hover);
			// gist::debug_view::eval_overlay(data, slot_data, uv_local, hover);
			break;
		}
	}

	debug_view::set_cursor_hover_data(data, hover);

	uint32 cursor_count = max(debug_view::load_cursor_data_count<true>(data), 1u);

	// todo, add multi select
	if (data.release_focus())
	{
		cursor_count = 1u;
	}
	else if (data.clicked())
	{
		if (debug_view::is_cursor_hit(data, hover))
		{
			debug_view::set_cursor_data(data, hover, 1);

			cursor_count = 2u;
		}
		else
		{
			cursor_count = 1u;
		}
	}

	debug_view_cursor_data src = hover;

	if (cursor_count > 1u)
	{
		const debug_view_cursor_data picked = debug_view::load_cursor_data<true>(data, 1);

		const bool valid = debug_view::is_cursor_hit(data, picked)
					   and picked.slot_id < data.slot_count
					   and debug_view::slot::enabled(debug_view::load_slot_data(data, picked.slot_id));

		if (valid)
		{
			src = picked;
		}
		else
		{
			cursor_count = 1u;
		}
	}

	debug_view::set_cursor_data_count(data, cursor_count);

	debug_view_popup_data popup_data;
	popup_data.pick_count = cursor_count - 1u;
	popup_data.rect		  = zero<int32_4>();

	if (debug_view::is_cursor_hit(data, src))
	{
		const debug_view_slot_data src_slot = debug_view::load_slot_data(data, src.slot_id);

		if (src_slot.system_popup_view_kind != 0u)
		{
			const int32_4 src_rect = debug_view::load_slot_rect<true>(data, src.slot_id);

			popup_data.rect = debug_view::calc_popup_rect(src_rect.xy + src.px_local, data.popup_view_size_uv, extent);
		}
	}

	debug_view::set_popup_data(data, popup_data);
}