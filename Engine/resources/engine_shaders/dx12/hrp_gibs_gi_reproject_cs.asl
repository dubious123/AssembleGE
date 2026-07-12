#include "hrp_common.asli"

#define GIBS_GI_MAX_AGE 16

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

wave_size(32)
[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	uint32_2 extent_dst = uint32_2(backbuffer_size.x, backbuffer_size.y);

	if (thread_id.x >= extent_dst.x or thread_id.y >= extent_dst.y) { return; }

	const int32_2 px = int32_2(thread_id.xy);

	rw_texture_2d<uint32>	gi_resolve_age_curr_buffer = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_uav_id];
	texture_2d<uint32>		gi_resolve_age_prev_buffer = global_resource_buffer[data.h_gi_resolve_age_prev_buffer_srv_id];
	rw_texture_2d<uint32_2> gi_resolve_geo_curr_buffer = global_resource_buffer[data.h_gi_resolve_geo_curr_buffer_uav_id];
	texture_2d<uint32_2>	gi_resolve_geo_prev_buffer = global_resource_buffer[data.h_gi_resolve_geo_prev_buffer_srv_id];

	rw_texture_2d<float3> gi_resolve_curr_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_uav_id];
	texture_2d<float3>	  gi_resolve_prev_buffer = global_resource_buffer[data.h_gi_resolve_prev_buffer_srv_id];

	texture_2d<float>	 depth_buffer  = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	   = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float2>	 motion_buffer = global_resource_buffer[motion_buffer_srv_id];

	float2 motion = sample_motion(motion_buffer, depth_buffer, px, extent_dst);
	// const float2 motion = motion_buffer[px];

	// float2 motion = motion_buffer[px];
	if (dot(motion, motion) < 0.01f * 0.01f) { motion = float2(0, 0); }
	// const float2 motion = zero<float2>();

	const float px_depth = depth_buffer[px];

	if (px_depth == 0.f)
	{
		// rr_irradiance_curr_buffer[px] = zero<float3>();
		gi_resolve_geo_curr_buffer[px] = zero<uint32_2>();
		gi_resolve_curr_buffer[px]	   = zero<float3>();
		gi_resolve_age_curr_buffer[px] = 0xff;
		return;
	}

	const uint32 px_normal_packed = gbuffer[px].y;
	const float3 px_normal		  = decode_oct_snorm16(px_normal_packed);
	const float	 px_z_lin		  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

	const float2  uv_prev	   = (float2(px) + 0.5f) * inv_backbuffer_size - motion;
	const float2  screen_pos_f = uv_prev * backbuffer_size;
	const int32_2 px_prev	   = clamp(int32_2(floor(screen_pos_f)), zero<int32_2>(), int32_2(extent_dst) - 1);

	const uint32_2 geo_prev		  = gi_resolve_geo_prev_buffer[px_prev];
	const float3   px_normal_prev = decode_oct_snorm16(geo_prev.x);
	const float	   px_depth_prev  = as_float(geo_prev.y);
	// const float	   ao_prev		  = f16tof32((geo_prev.y >> 16u) & 0x7fff);

	const float3 world_pos_curr = ndc_to_world(view_proj_inv, screen_to_ndc(float2(px) + 0.5f, px_depth, inv_backbuffer_size));
	const float3 world_pos_prev = ndc_to_world(view_proj_inv_prev, screen_to_ndc(float2(px_prev) + 0.5f, px_depth_prev, inv_backbuffer_size));

	const bool is_prev_valid = all(uv_prev >= 0.f)
						   and all(uv_prev < 1.f)
						   and (geo_prev.y != 0u)
						   and check_prev_valid(world_pos_prev, world_pos_curr, px_normal_prev, px_normal, px_z_lin);

	float ao = 1.f;

	if (ao::enabled())
	{
		const ao_data	   ao_data	 = ao::load_data();
		texture_2d<float4> ao_buffer = global_resource_buffer[ao_data.h_ao_buffer_srv_id];
		const float		   ao_curr	 = ao_buffer[px].x;

		if (is_prev_valid)
		{
			// ao = lerp(ao_prev, ao_curr, 1.f / (ao_data.slice_count() * ao_data.offset_count()));

			ao = ao_curr;
		}
		else
		{
			ao = ao_curr;
		}
	}

	// todo, ndc depth -> linear z

	// geo_curr_buffer[px] = uint32_2(px_normal_packed, f32tof16(px_depth) | (f32tof16(ao) << 16u));

	// gi_resolve_geo_curr_buffer[px] = uint32_2(px_normal_packed, f32tof16(px_depth) | ((f32tof16(ao) & 0x7fffu) << 16u) | ((1u) << 31u));
	gi_resolve_geo_curr_buffer[px] = uint32_2(px_normal_packed, as_uint(px_depth));

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

		float3 gi_val[4];
		float3 normal[4];
		float  z_lin[4];
		bool   valid[4];
		uint16 gi_age[4];

		const float2 ratio[4] = {
			float2(1.f - f.x, 1.f - f.y),
			float2(f.x, 1.f - f.y),
			float2(1.f - f.x, f.y),
			float2(f.x, f.y),
		};

		for (int32 i = 0; i < 4; ++i)
		{
			const uint32_2 geo = gi_resolve_geo_prev_buffer[tap[i]];
			gi_val[i]		   = gi_resolve_prev_buffer[tap[i]];
			gi_age[i]		   = cast<uint16>(gi_resolve_age_prev_buffer[tap[i]]);
			normal[i]		   = decode_oct_snorm16(geo.x);
			z_lin[i]		   = calc_linear_z_reversed(cam_near_z, cam_far_z, as_float(geo.y));
			valid[i]		   = geo.y != 0u;
		}

		float4 gi = zero<float4>();

		float gi_age_sum	 = 0.f;
		float gi_age_quality = 0.f;
		for (int32 i = 0; i < 4; ++i)
		{
			if (valid[i])
			{
				const float w = calc_bilateral_weight(px_z_lin, px_normal, z_lin[i], normal[i], ratio[i]);

				if (w > 0.f)
				{
					gi_age_sum += gi_age[i] * w;
					gi		   += float4(gi_val[i], 1.f) * w;

					gi_age_quality += ratio[i].x * ratio[i].y;
				}
			}
		}


		if (gi.w > 0.f)
		{
			gi_resolve_curr_buffer[px] = gi.xyz / gi.w;

			const float	 gi_age_avg		   = gi_age_sum / gi.w + 1.f;
			const float	 gi_age_res		   = gi_age_quality < 1.f ? gi_age_avg * sqrt(gi_age_quality) : gi_age_avg;
			const uint16 gi_age_res_i	   = clamp(uint16(round(gi_age_res)), uint16(1), uint16(GIBS_GI_MAX_AGE));
			gi_resolve_age_curr_buffer[px] = gi_age_res_i;
		}
		else
		{
			gi_resolve_curr_buffer[px] = gi_resolve_prev_buffer[px_prev];

			// gi_resolve_curr_buffer[px] = color_red.xyz;
			gi_resolve_age_curr_buffer[px] = 0u;
		}


		// rr_irradiance_curr_buffer[px] = rr_irradiance_prev_buffer[px_prev];
	}
	else
	{
		gi_resolve_age_curr_buffer[px] = 0u;

		const float4 gi_irradiance_sum = gibs_gather_neighbor_gi(gi_resolve_prev_buffer, gi_resolve_geo_prev_buffer, px, px_z_lin, px_normal, extent_dst);

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
