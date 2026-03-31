#include "forward_plus_common.asli"

float4
main_ps(ui_ms_to_ps ps_in) sv_target_0
{
	const ui_data data = load_ui_data(ps_in.ui_data_id);

	const float2 px_offset	   = ps_in.pos.xy - data.pivot_pos;
	const float2 px_rotated	   = rotate(px_offset, -data.rotation);
	const float2 center_offset = px_rotated - (float2(0.5f, 0.5f) - data.pivot_uv) * data.size;

	float  delta_from_edge = 0.f;
	float3 body_color	   = float3(1, 1, 1);
	float3 border_color	   = float3(1, 1, 1);

	const uint32 shape_kind		   = (data.packed_enums >> 0) & 0xff;
	const uint32 body_brush_kind   = (data.packed_enums >> 8) & 0xff;
	const uint32 border_brush_kind = (data.packed_enums >> 16) & 0xff;

	switch (shape_kind)
	{
	case UI_SHAPE_KIND_RECT:
	{
		delta_from_edge = ui_calc_shape_rect(center_offset, data.size, data.shape_data);
		break;
	}
	case UI_SHAPE_KIND_CIRCLE:
	{
		delta_from_edge = ui_calc_shape_circle(center_offset, data.size, data.shape_data);
		break;
	}
	case UI_SHAPE_KIND_ARROW_RIGHT:
	{
		delta_from_edge = ui_calc_shape_arrow_right(center_offset, data.size, data.shape_data);
		break;
	}
	}

	switch (body_brush_kind)
	{
	case UI_BRUSH_KIND_COLOR:
	{
		body_color = ui_calc_brush_color(ps_in.rect_uv, data.body_brush_data);
		break;
	}
	}

	switch (border_brush_kind)
	{
	case UI_BRUSH_KIND_COLOR:
	{
		border_color = ui_calc_brush_color(ps_in.rect_uv, data.border_brush_data);
		break;
	}
	}


	// 0 ~ -border + 1 : border
	// -border + 1 ~ -border - 1 : lerp
	// -border - 1 ~ 0 : body

	float aa = fwidth(delta_from_edge);

	float outer_alpha = 1.0 - smoothstep(-aa, aa, delta_from_edge);
	float inner_alpha = 1.0 - smoothstep(-aa, aa, delta_from_edge + data.border_thickness);

	if (ps_in.pos.x >= data.clip_rect.x
		&& ps_in.pos.y >= data.clip_rect.y
		&& ps_in.pos.x <= data.clip_rect.z
		&& ps_in.pos.y <= data.clip_rect.w)
	{
		return float4(lerp(border_color, body_color, inner_alpha), outer_alpha);
	}
	else
	{
		return float4(lerp(border_color, body_color, inner_alpha), 0);
	}
}