#include "hrp_common.asli"

groupshared ddgi_as_to_ms as_out;

[numthreads(32, 1, 1)] void
main_as(
	uint32_3 dispatch_thread_id sv_dispatch_thread_id,
	uint32_3 group_id			sv_group_id,
	uint32_3 group_thread_id	sv_group_thread_id)

{
	const ddgi_data ddgi_data = load_ddgi_data();

	const uint32 local_probe_id = dispatch_thread_id.x;
	const uint32 level			= group_id.z;
	const uint32 probe_id		= local_probe_id | (level << load_ddgi_ppl_log2(ddgi_data));

	const float3 probe_pos = ddgi_calc_probe_pos(ddgi_data, probe_id, level);

	texture_2d<float> tex_depth = global_resource_buffer[depth_buffer_texture_id];

	const float4 clip_pos = mul(view_proj, float4(probe_pos, 1.f));
	const float3 ndc	  = clip_pos.xyz / clip_pos.w;

	const float2 screen_uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;

	const float depth = sample_level(tex_depth, get_point_clamp_sampler(), screen_uv, 0);

	bool visible = clip_pos.w > 0.f
			   and not(any(abs(ndc.xy) > 1.0f) or ndc.z < 0.0f or ndc.z > 1.0f or ndc.z < depth);

	// visible &= level < 3;

	attr_branch()

	if (ddgi_debug_flags_render_probe_in_hole(ddgi_data) is_false)
	{
		visible &= not ddgi_is_probe_in_hole(ddgi_data, probe_pos, level);
	}

	// const bool visible = true;

	const uint32_4 ballot		= wave_active_ballot(visible);
	uint32		   visible_mask = ballot.x;

	// todo, add option to cull probe based on probe states

	if (wave_is_first_lane())
	{
		as_out.probe_id_begin	= probe_id;
		as_out.probe_alive_mask = visible_mask;
	}

	dispatch_mesh(countbits(visible_mask), 1, 1, as_out);
}