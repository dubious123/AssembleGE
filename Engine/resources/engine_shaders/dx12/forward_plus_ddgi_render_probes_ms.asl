#include "forward_plus_common.asli"


[numthreads(32, 1, 1)][output_topology("triangle")] void
main_ms(
	in payload ddgi_as_to_ms   ms_in,
	uint32_3 group_id		   sv_group_id,
	uint32_3 group_thread_id   sv_group_thread_id,
	out vertices ddgi_ms_to_ps ms_out_vertex_arr[6],
	out indices uint32_3	   ms_out_triangle_arr[8])

{
	const uint32 not_culled_probe_local_index = select32_nth_set_bit(ms_in.probe_alive_mask, group_id.x);
	const uint32 probe_id					  = ms_in.probe_id_begin + not_culled_probe_local_index;

	// octahedron
	const uint32 vertex_count	= 6;
	const uint32 triangle_count = 8;

	set_mesh_output_counts(vertex_count, triangle_count);

	const ddgi_data ddgi_data = load_ddgi_data();
	const uint32	level	  = probe_id >> load_ddgi_ppl_log2(ddgi_data);
	// const ddgi_probe probe	   = load_ddgi_probe_srv(probe_id);
	const float3 probe_pos = ddgi_calc_probe_pos(ddgi_data, probe_id, level) /*+ cast<float3>(probe.offset)*/;
	const float	 radius	   = 0.1f * float(1u << level);

	static const float3 octahedron_vertices[6] = {
		float3(1.0f, 0.0f, 0.0f),	  // +X
		float3(-1.0f, 0.0f, 0.0f),	  // -X
		float3(0.0f, 1.0f, 0.0f),	  // +Y
		float3(0.0f, -1.0f, 0.0f),	  // -Y
		float3(0.0f, 0.0f, 1.0f),	  // +Z
		float3(0.0f, 0.0f, -1.0f)	  // -Z
	};

	static const uint3 octahedron_triangles[8] = {
		uint3(0, 2, 4),				  // +X +Y +Z
		uint3(0, 4, 3),				  // +X +Z -Y
		uint3(0, 3, 5),				  // +X -Y -Z
		uint3(0, 5, 2),				  // +X -Z +Y
		uint3(1, 4, 2),				  // -X +Z +Y
		uint3(1, 3, 4),				  // -X -Y +Z
		uint3(1, 5, 3),				  // -X -Z -Y
		uint3(1, 2, 5)				  // -X +Y -Z
	};

	if (group_thread_id.x < vertex_count)
	{
		const float3 unit_pos  = octahedron_vertices[group_thread_id.x];
		const float3 world_pos = probe_pos + unit_pos * radius;
		const float4 clip_pos  = mul(view_proj, float4(world_pos, 1.f));

		ddgi_ms_to_ps v = (ddgi_ms_to_ps)0;
		v.probe_id		= probe_id;
		v.pos			= clip_pos;
		v.world_pos		= world_pos;
		v.normal		= unit_pos;

		ms_out_vertex_arr[group_thread_id.x] = v;
	}

	if (group_thread_id.x < triangle_count)
	{
		ms_out_triangle_arr[group_thread_id.x] = octahedron_triangles[group_thread_id.x];
	}
}