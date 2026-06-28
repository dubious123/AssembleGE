#include "hrp_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	texture_2d<float4> main_tex = global_resource_buffer[main_buffer_texture_id];

	float3 col = load(main_tex, pos.x, pos.y, 0).rgb;

	col = max(col, 0.0);

	const bloom bloom = load_bloom();

	attr_branch()

	if (bloom.srv_texture_id != invalid_id_uint32)
	{
		texture_2d<float3> bloom_tex	= global_resource_buffer[bloom.srv_texture_id];
		float3			   bloom_color	= sample_level(bloom_tex, get_linear_clamp_sampler(), pos.xy * inv_backbuffer_size, 0).rgb;
		col							   += bloom_color * bloom.intensity * bloom.tint;
	}


	if (gibs_enabled())
	{
		const gibs_data data	 = gibs_load_gibs_data();
		float2			debug_uv = pos.xy * inv_backbuffer_size;

		if (debug_uv.x > 0.75 and debug_uv.y < 0.25)
		{
			float2	uv		   = (debug_uv - float2(0.75f, 0.f)) * 4;
			int32_2 screen_pos = uv * backbuffer_size;

			texture_2d<uint32_2> gbuffer		   = global_resource_buffer[gbuffer_srv_id];
			texture_2d<float>	 depth_buffer	   = global_resource_buffer[depth_buffer_texture_id];
			texture_2d<float3>	 gi_resolve_buffer = global_resource_buffer[data.h_gi_resolve_full_res_buffer_srv_id];
			const float			 z_depth		   = load(depth_buffer, screen_pos);
			const float3		 px_normal		   = max(float3(0, 0, 0), decode_oct_snorm16(load(gbuffer, screen_pos).y));

			col = px_normal;
		}
		else if (debug_uv.x > 0.9 and debug_uv.y < 0.26)
		{
			const float ratio = gibs_load_ray_count_total(data) / (float)GIBS_RAY_BUDGET;
			float2		uv	  = (debug_uv - float2(0.9f, 0.25f)) * 10;

			if (uv.x < ratio)
			{
				col = float3(ratio, 1 - ratio, 0);
			}
			else
			{
			}
		}

		else if (debug_uv.x > 0.8 and debug_uv.x < 0.9 and debug_uv.y < 0.26)
		{
			rw_stack<uint32> prev  = gibs_load_alive_surfel_id_stack_prev(data);
			rw_stack<uint32> dead  = gibs_load_dead_surfel_id_stack(data);
			rw_stack<uint32> curr  = gibs_load_alive_surfel_id_stack_curr(data);
			const float		 ratio = curr.size() / (float)data.max_surfel_count;
			float2			 uv	   = (debug_uv - float2(0.8f, 0.25f)) * 10;

			if (uv.x < ratio)
			{
				col = float3(ratio, 1 - ratio, 0);
			}
			else
			{
			}
		}
		else if (debug_uv.x > 0.8 and debug_uv.x < 0.9 and debug_uv.y < 0.27)
		{
			const float ratio = gibs_load_tile_surfel_count(data) / ((float)data.max_surfel_count * 9);
			float2		uv	  = (debug_uv - float2(0.8f, 0.26f)) * 10;

			assert(ratio <= 1.f, g::fmt_forward_plus_post_process_ps, line, gibs_load_tile_surfel_count(data));

			if (uv.x < ratio)
			{
				col = float3(ratio, 1 - ratio, 0);
			}
			else
			{
			}
		}
		else if (debug_uv.x > 0.8 and debug_uv.x < 0.9 and debug_uv.y < 0.28)
		{
			const float ratio = gibs_load_cell_surfel_count(data) / ((float)data.max_surfel_count * 27);
			float2		uv	  = (debug_uv - float2(0.8f, 0.27f)) * 10;

			assert(ratio <= 1.f, g::fmt_forward_plus_post_process_ps, line, gibs_load_cell_surfel_count(data));

			if (uv.x < ratio)
			{
				col = float3(ratio, 1 - ratio, 0);
			}
			else
			{
			}
		}
		else if (debug_uv.x > 0.5 and debug_uv.x < 0.75 and debug_uv.y < 0.25)
		{
			texture_2d<float3> gi_resolve = global_resource_buffer[data.h_gi_resolve_full_res_buffer_srv_id];
			float2			   uv		  = (debug_uv - float2(0.5f, 0.f)) * 4;
			// font_uv.y	   = 1.f - font_uv.y;
			col = sample_level(gi_resolve, get_linear_clamp_sampler(), uv, 0);
		}
	}

	if (ao::enabled())
	{
		float2 debug_uv = pos.xy * inv_backbuffer_size;
		if (debug_uv.x > 0.75 and debug_uv.y < 0.25)
		{
			float2	uv		   = (debug_uv - float2(0.75f, 0.f)) * 4;
			int32_2 screen_pos = uv * backbuffer_size;

			texture_2d<float4> ao_buffer = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];

			float4 ao_res = ao_buffer[screen_pos];

			ao_res.yz = ao_res.yz * 2.f - 1.f;

			col = ao_res.xyz;
			col = float3(1.f - ao_res.x, 1.f - ao_res.x, 1.f - ao_res.x) * 10;
			col = float3(ao_res.x, ao_res.x, ao_res.x);
		}
	}
	// else
	//{
	//	float2 debug_uv = pos.xy * inv_backbuffer_size;
	//	if (debug_uv.x > 0.75 and debug_uv.y < 0.25)
	//	{
	//		float2	uv		   = (debug_uv - float2(0.75f, 0.f)) * 4;
	//		int32_2 screen_pos = uv * backbuffer_size;

	//		texture_2d<float2> motion_buffer = global_resource_buffer[motion_buffer_srv_id];

	//		float2 motion = motion_buffer[screen_pos];

	//		float2 m = motion * 100;
	//		col		 = float3(m * 0.5f + 0.5f, 0.5f);

	//		// col = motion.xyz;
	//		// col = float3(1.f - motion.x, 1.f - motion.x, 1.f - motion.x) * 10;
	//		// col = float3(motion.x, motion.x, motion.x);
	//	}
	//}
	else
	{
		float2 debug_uv = pos.xy * inv_backbuffer_size;
		if (debug_uv.x > 0.75 and debug_uv.y < 0.25)
		{
			const float2 uv = (debug_uv - float2(0.75f, 0.f)) * 4;

			// const float2 uv = debug_uv;

			const int32_2 screen_pos = uv * backbuffer_size;

			col = segment_is_edge(screen_pos) ? color_red.xyz : color_white.xyz;
		}
	}


	// col = tonemap_reinhard_luminance(col, 4.0);
	col = tonemap_aces_hill_hdr(col, 4.0);

	return float4(col, 1.0);
}