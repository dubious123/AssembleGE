#include "forward_plus_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	texture_2d<uint32> mask_tex = global_resource_buffer[selection_outline_mask_buffer_srv_texture_id];

	uint32 id_self = load(mask_tex, pos.x, pos.y, 0) & 0xff;

	uint32 id_res			= 0xff;
	float  distance_sqr_res = 9999.f;

	expand_all()

	for (int32 dy = -MAX_SELECTION_OUTLINE_THICKNESS; dy <= MAX_SELECTION_OUTLINE_THICKNESS; ++dy)
	{
		for (int32 dx = -MAX_SELECTION_OUTLINE_THICKNESS; dx <= MAX_SELECTION_OUTLINE_THICKNESS; ++dx)
		{
			if (dx == 0 && dy == 0) { continue; }

			uint32 selection_outline_id = load(mask_tex, pos.x + dx, pos.y + dy, 0) & 0xff;
			if (selection_outline_id == 0xff or selection_outline_id == id_self) { continue; }

			const selection_outline_data data = load_selection_outline_data(selection_outline_id);

			const float thickness = cast<float>(data.thickness_and_softness & 0xff) / (1 << 6);

			float distance_sqr = float(dx * dx + dy * dy);
			if (distance_sqr > thickness * thickness) { continue; }

			if (distance_sqr < distance_sqr_res)
			{
				id_res			 = selection_outline_id;
				distance_sqr_res = distance_sqr;
			}
		}
	}

	if (id_res == 0xff or id_res == id_self)
	{
		discard;
		return float4(0, 0, 0, 0);
	}
	else
	{
		const selection_outline_data data = load_selection_outline_data(id_res & 0xff);

		const float thickness = cast<float>(data.thickness_and_softness & 0xff) / (1 << 6);
		const float softness  = unorm8_to_float((data.thickness_and_softness >> 8) & 0xff);

		const float dist  = sqrt(distance_sqr_res);
		const float alpha = saturate((thickness - dist) / max(thickness * softness, 1.f / (1 << 6)));

		return float4(data.rgba.xyz, data.rgba.w * alpha);
	}
}