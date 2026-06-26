#include "hrp_common.asli"

DECLARE_CALC_THREAD_GROUP_SUM(32)

[numthreads(32, 1, 1)][output_topology("triangle")] void
main_ms(
	uint32 group_id		   sv_group_id,
	uint32 group_thread_id sv_group_thread_id,

	out vertices ui_ms_to_ps vertex_arr[128],
	out indices uint32_3	 triangle_arr[64])

{
	const uint32 ui_data_id = ui_data_id_offset + group_id * 32 + group_thread_id;
	const uint32 valid		= group_id * 32 + group_thread_id < ui_data_count;

	uint32 group_ui_count = 0;

	group_ui_count = wave_active_sum(valid);

	set_mesh_output_counts(group_ui_count * 4, group_ui_count * 2);

	if (valid is_false)
	{
		return;
	}

	const ui_data data = load_ui_data(ui_data_id);

	static float2 corner_uv[4] = {
		float2(0, 0), float2(1, 0),
		float2(0, 1), float2(1, 1)
	};

	expand(4)

	for (uint32 i = 0; i < 4; ++i)
	{
		ui_ms_to_ps v;

		v.ui_data_id = ui_data_id;

		v.rect_uv = corner_uv[i];

		attr_branch()

		switch (ui_space_mode_and_extra & 0xff)
		{
		case UI_SPACE_MODE_SCREEN:
		{
			v.pos.xy = screen_to_ndc(data.pivot_pos + rotate((corner_uv[i] - data.pivot_uv) * data.size, data.rotation), inv_backbuffer_size);
			v.pos.z	 = 1;
			v.pos.w	 = 1;
			break;
		}
		case UI_SPACE_MODE_WORLD:
		case UI_SPACE_MODE_WORLD_ALWAYS_ON_TOP:
		{
			const ui_root_data root = load_ui_root_data();

			const float2 local_uv = data.pivot_pos + rotate((corner_uv[i] - data.pivot_uv) * data.size, data.rotation);

			const float3 world_pos = root.world_pos
								   + rotate(root.quaternion, float3(1, 0, 0)) * local_uv.x * root.world_width / root.width
								   + rotate(root.quaternion, float3(0, -1, 0)) * local_uv.y * root.world_height / root.height;

			v.pos = mul(view_proj, float4(world_pos, 1.f));
			break;
		}
		default:
		{
			break;
		}
		}

		vertex_arr[group_thread_id * 4 + i] = v;
	}


	triangle_arr[group_thread_id * 2]	  = uint32_3(group_thread_id * 4 + 0, group_thread_id * 4 + 1, group_thread_id * 4 + 3);
	triangle_arr[group_thread_id * 2 + 1] = uint32_3(group_thread_id * 4 + 0, group_thread_id * 4 + 3, group_thread_id * 4 + 2);
}