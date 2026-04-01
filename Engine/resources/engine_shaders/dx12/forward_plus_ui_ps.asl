#include "forward_plus_common.asli"

float
screen_px_range(float2 atlas_uv, float2 atlas_size, float px_range)
{
	const float2 unit_range		 = px_range / atlas_size;
	const float2 dx				 = ddx(atlas_uv);
	const float2 dy				 = ddy(atlas_uv);
	const float2 screen_tex_size = rsqrt(dx * dx + dy * dy);
	return max(0.5f * dot(unit_range, screen_tex_size), 1.0f);
}

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

	float sd;

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
	case UI_SHAPE_KIND_TEXT:
	{
		const float2 atlas_uv_min = float2(as_float(data.shape_data.data[0]), as_float(data.shape_data.data[1]));
		const float2 atlas_uv_max = float2(as_float(data.shape_data.data[2]), as_float(data.shape_data.data[3]));
		const uint32 atlas_id	  = data.shape_data.data[4];
		// const float2 atlas_uv	  = lerp(atlas_uv_min, atlas_uv_max, ps_in.rect_uv);
		const float2 atlas_uv = lerp(atlas_uv_min, atlas_uv_max, float2(ps_in.rect_uv.x, 1.0f - ps_in.rect_uv.y));
		//  const float2	   atlas_uv = float2(ps_in.rect_uv.x, 1.0f - ps_in.rect_uv.y);
		texture_2d<float4> atlas = global_resource_buffer[atlas_id];
		float2			   atlas_size;
		atlas.GetDimensions(atlas_size.x, atlas_size.y);
		const float4 rgba = sample(atlas, linear_clamp_sampler, atlas_uv);
		// const float4 rgba = sample(atlas, linear_clamp_sampler, ps_in.rect_uv);

		sd				= max(min(rgba.r, rgba.g), min(max(rgba.r, rgba.g), rgba.b));
		delta_from_edge = (0.5f - sd) * screen_px_range(atlas_uv, atlas_size, 2.f);
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
		if (shape_kind == UI_SHAPE_KIND_TEXT)
		{
			return float4(1, 1, 1, sd);
		}

		return float4(lerp(border_color, body_color, inner_alpha), outer_alpha);
	}
	else
	{
		return float4(lerp(border_color, body_color, inner_alpha), 0);
	}
}