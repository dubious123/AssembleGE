#include "hrp_common.asli"

bool
check_prev_valid(float3 pos_prev, float3 pos_curr, float3 normal_prev, float3 normal_curr, float px_z_lin)
{
	const float cos_theta	  = dot(normal_prev, normal_curr);
	const float dist_to_plane = abs(dot(pos_curr - pos_prev, normal_curr));

	const float3 view_dir				= normalize(camera_pos - pos_curr);
	const float	 n_dot_v				= max(dot(normal_curr, view_dir), 0.1f);
	const float	 disocclusion_threshold = 0.001f;
	const float	 threshold				= disocclusion_threshold * px_z_lin / n_dot_v;

	return cos_theta > (1.f - 0.1f) and dist_to_plane < threshold;
};

float2
sample_motion(texture_2d<float2> motion_buffer, texture_2d<float> depth_buffer, int32_2 px, int32_2 extent)
{
	// reversed_z
	float	closest_depth = 0.f;
	int32_2 closest_px	  = px;

	for (int32 dy = -1; dy <= 1; ++dy)
	{
		for (int32 dx = -1; dx <= 1; ++dx)
		{
			const int32_2 tap		   = clamp(px + int32_2(dx, dy), zero<int32_2>(), int32_2(extent) - 1);
			const float	  px_depth_tap = depth_buffer[tap];

			if (px_depth_tap > closest_depth)
			{
				closest_depth = px_depth_tap;
				closest_px	  = tap;
			}
		}
	}

	return motion_buffer[closest_px];
};

float4
gibs_sample_bilateral_bilinear(float3 v[4], float z_lin[4], float3 normal[4], bool valid[4], float2 f, float center_z_lin, float3 center_n)
{
	const float2 ratio[4] = {
		float2(1.f - f.x, 1.f - f.y),
		float2(f.x, 1.f - f.y),
		float2(1.f - f.x, f.y),
		float2(f.x, f.y),
	};

	float4 res = zero<float4>();

	for (int32 i = 0; i < 4; ++i)
	{
		if (valid[i])
		{
			res += float4(v[i], 1.f) * calc_bilateral_weight(center_z_lin, center_n, z_lin[i], normal[i], ratio[i]);
		}
	}

	return res;
}

wave_size(32)
[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	uint32_2 extent_dst = uint32_2(backbuffer_size.x, backbuffer_size.y);

	if (thread_id.x >= extent_dst.x or thread_id.y >= extent_dst.y) { return; }

	const int32_2 px = int32_2(thread_id.xy);

	rw_texture_2d<float3>	rr_irradiance_curr_buffer = global_resource_buffer[data.h_gi_resolve_rr_irradiance_curr_buffer_uav_id];
	texture_2d<float3>		rr_irradiance_prev_buffer = global_resource_buffer[data.h_gi_resolve_rr_irradiance_prev_buffer_srv_id];
	rw_texture_2d<uint32_2> rr_geo_curr_buffer		  = global_resource_buffer[data.h_gi_resolve_rr_geo_curr_buffer_uav_id];
	texture_2d<uint32_2>	rr_geo_prev_buffer		  = global_resource_buffer[data.h_gi_resolve_rr_geo_prev_buffer_srv_id];

	rw_texture_2d<float3> gi_resolve_curr_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_uav_id];
	texture_2d<float3>	  gi_resolve_prev_buffer = global_resource_buffer[data.h_gi_resolve_prev_buffer_srv_id];

	texture_2d<float>	 depth_buffer  = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	   = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float2>	 motion_buffer = global_resource_buffer[motion_buffer_srv_id];

	const float2 motion = sample_motion(motion_buffer, depth_buffer, px, extent_dst);

	const float px_depth = depth_buffer[px];

	if (px_depth == 0.f)
	{
		rr_irradiance_curr_buffer[px] = zero<float3>();
		rr_geo_curr_buffer[px]		  = uint32_2(0, 0x7fff0000);
		gi_resolve_curr_buffer[px]	  = zero<float3>();
		return;
	}

	const int32_2  block_px = (px / GIBS_GI_RESOLVE_SCALE) * GIBS_GI_RESOLVE_SCALE;
	const uint32_2 fresh_px = uint32_2(block_px) + gibs_calc_rr_offset(block_px, extent_dst, frame_index);
	const bool	   is_fresh = all(uint32_2(px) == fresh_px);

	const uint32 px_normal_packed = gbuffer[px].y;
	const float3 px_normal		  = decode_oct_snorm16(px_normal_packed);
	const float	 px_z_lin		  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

	const float2  uv_prev	   = (float2(px) + 0.5f) * inv_backbuffer_size - motion;
	const float2  screen_pos_f = uv_prev * backbuffer_size;
	const int32_2 px_prev	   = clamp(int32_2(floor(uv_prev * backbuffer_size)), zero<int32_2>(), int32_2(extent_dst) - 1);

	const uint32_2 geo_prev		  = rr_geo_prev_buffer[px_prev];
	const float3   px_normal_prev = decode_oct_snorm16(geo_prev.x);
	const float	   px_depth_prev  = f16tof32(geo_prev.y & 0xffff);
	const float	   ao_prev		  = f16tof32((geo_prev.y >> 16u) & 0x7fff);

	const float3 world_pos_curr = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px) + 0.5f, px_depth, inv_backbuffer_size));
	const float3 world_pos_prev = ndc_to_world(view_proj_inv_prev, screen_to_ndc(float2(px_prev) + 0.5f, px_depth_prev, inv_backbuffer_size));

	const bool is_prev_valid = all(uv_prev >= 0.f)
						   and all(uv_prev < 1.f)
						   and ((geo_prev.y >> 31u) & 1u)
						   and check_prev_valid(world_pos_prev, world_pos_curr, px_normal_prev, px_normal, px_z_lin);

	float ao = 1.f;

	if (ao::enabled())
	{
		const ao_data	   ao_data	 = ao::load_data();
		texture_2d<float4> ao_buffer = global_resource_buffer[ao_data.h_ao_buffer_srv_id];
		const float		   ao_curr	 = ao_buffer[px].x;

		if (is_prev_valid)
		{
			ao = lerp(ao_prev, ao_curr, 1.f / (ao_data.slice_count() * ao_data.offset_count()));
		}
		else
		{
			ao = ao_curr;
		}
	}

	// todo, ndc depth -> linear z

	const uint32 valid = (is_prev_valid or is_fresh) ? 1u : 0u;
	// geo_curr_buffer[px] = uint32_2(px_normal_packed, f32tof16(px_depth) | (f32tof16(ao) << 16u));

	rr_geo_curr_buffer[px] = uint32_2(px_normal_packed, f32tof16(px_depth) | (f32tof16(ao) << 16u) | (valid << 31u));

	if (is_prev_valid)
	{
		const float2  base_f = screen_pos_f - 0.5f;
		const int32_2 base_i = int32_2(floor(base_f));
		const float2  f		 = frac(base_f);

		const int32_2 tap[4] = {
			clamp(base_i, zero<int32_2>(), int32_2(extent_dst) - 1),
			clamp(base_i + int32_2(1, 0), zero<int32_2>(), int32_2(extent_dst) - 1),
			clamp(base_i + int32_2(0, 1), zero<int32_2>(), int32_2(extent_dst) - 1),
			clamp(base_i + int32_2(1, 1), zero<int32_2>(), int32_2(extent_dst) - 1),
		};

		float3 rr_val[4];
		float3 gi_val[4];
		float3 normal[4];
		float  z_lin[4];
		bool   valid[4];

		for (int32 i = 0; i < 4; ++i)
		{
			const uint32_2 geo = rr_geo_prev_buffer[tap[i]];
			rr_val[i]		   = rr_irradiance_prev_buffer[tap[i]];
			gi_val[i]		   = gi_resolve_prev_buffer[tap[i]];
			normal[i]		   = decode_oct_snorm16(geo.x);
			z_lin[i]		   = calc_linear_z_reversed(cam_near_z, cam_far_z, f16tof32(geo.y & 0xffff));
			valid[i]		   = (geo.y >> 31u) != 0u;
		}

		const float4 rr = gibs_sample_bilateral_bilinear(rr_val, z_lin, normal, valid, f, px_z_lin, px_normal);

		if (rr.w > 0.f)
		{
			rr_irradiance_curr_buffer[px] = rr.xyz / rr.w;
		}
		else
		{
			rr_irradiance_curr_buffer[px] = rr_irradiance_prev_buffer[px_prev];
			rr_irradiance_curr_buffer[px] = color_red.xyz;
		}


		const float4 gi = gibs_sample_bilateral_bilinear(gi_val, z_lin, normal, valid, f, px_z_lin, px_normal);

		if (gi.w > 0.f)
		{
			gi_resolve_curr_buffer[px] = gi.xyz / gi.w;
		}
		else
		{
			gi_resolve_curr_buffer[px] = gi_resolve_prev_buffer[px_prev];
			gi_resolve_curr_buffer[px] = color_red.xyz;
		}


		// rr_irradiance_curr_buffer[px] = rr_irradiance_prev_buffer[px_prev];
	}
	else
	{
		const float4 rr_irradiance_sum = gibs_gather_neighbor_gi(rr_irradiance_prev_buffer, rr_geo_prev_buffer, px, px_z_lin, px_normal, extent_dst);
		if (rr_irradiance_sum.w > 0.f)
		{
			rr_irradiance_curr_buffer[px] = rr_irradiance_sum.xyz / rr_irradiance_sum.w;
		}
		else
		{
			rr_irradiance_curr_buffer[px] = rr_irradiance_prev_buffer[px];

			// rr_irradiance_curr_buffer[px] = color_red.xyz;
		}
		// const uint32					 vis_packed	 = gbuffer[px].x;
		// const uint32					 render_id	 = vis_packed & 0x01ffffff;
		// const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_id);

		// irradiance_curr_buffer[px] = color_green.xyz;
		// irradiance_curr_buffer[px] = gibs_sample_screen_irradiance(fresh_px, render_data.object_id, world_pos_curr, px_normal);

		const float4 gi_irradiance_sum = gibs_gather_neighbor_gi(gi_resolve_prev_buffer, rr_geo_prev_buffer, px, px_z_lin, px_normal, extent_dst);

		if (gi_irradiance_sum.w > 0.f)
		{
			gi_resolve_curr_buffer[px] = gi_irradiance_sum.xyz / gi_irradiance_sum.w;
		}
		else
		{
			gi_resolve_curr_buffer[px] = gi_resolve_prev_buffer[px];

			// gi_resolve_curr_buffer[px] = color_red.xyz;
		}
	}
}
