#include "forward_plus_common.asli"

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

		v.pos.xy = screen_to_ndc(data.pivot_pos + rotate((corner_uv[i] - data.pivot_uv) * data.size, data.rotation), inv_backbuffer_size);
		v.pos.z	 = 1;
		v.pos.w	 = 1;


		vertex_arr[group_thread_id * 4 + i] = v;
	}


	triangle_arr[group_thread_id * 2]	  = uint32_3(group_thread_id * 4 + 0, group_thread_id * 4 + 1, group_thread_id * 4 + 3);
	triangle_arr[group_thread_id * 2 + 1] = uint32_3(group_thread_id * 4 + 0, group_thread_id * 4 + 3, group_thread_id * 4 + 2);
}