#include "hrp_common.asli"

#define GIBS_GI_SCATTER_LUM_SCALE 2

wave_size(AGE_WAVE_SIZE)
[numthreads(GIBS_GI_RESOLVE_BLOCK_SIZE, GIBS_GI_RESOLVE_BLOCK_SIZE, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	const int32_2 extent = int32_2(backbuffer_size);

	if (thread_id.x >= extent.x or thread_id.y >= extent.y) { return; }

	const int32_2 px = int32_2(thread_id.xy);

	array<gibs_gi_resolve_sample_res> gi_resolve_sample_res_arr = gibs_load_gi_resolve_sample_res_arr(data);

	rw_texture_2d<float3> gi_resolve_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_uav_id];

	texture_2d<uint32> gi_resolve_age_buffer = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_srv_id];

	rw_texture_2d<float> gi_resolve_weight_buffer = global_resource_buffer[data.h_gi_resolve_weight_buffer_uav_id];

	const uint16 age_curr = cast<uint16>(gi_resolve_age_buffer[px]);

	texture_2d<float>	 depth_buffer	= global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> opaque_gbuffer = global_resource_buffer[opaque_gbuffer_srv_id];
	const float3		 px_normal_curr = decode_oct_snorm16(opaque_gbuffer[px].y);
	const float			 z_depth		= depth_buffer[px];
	// sky
	if (z_depth == 0.f)
	{
		gi_resolve_buffer[px] = zero<float3>();
		return;
	}

	const float px_z_lin_curr = calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth);

	const float3 px_irradiance	= gi_resolve_buffer[px];
	const float	 px_luminance	= luminance_rec709(px_irradiance);
	float4		 irradiance_sum = zero<float4>();
	{
		const int32_2 block_idx	   = px / GIBS_GI_RESOLVE_BLOCK_SIZE;
		const int32_2 block_extent = int32_2(data.tile_count_w * GIBS_GI_RESOLVE_BLOCK_DIM, data.tile_count_h * GIBS_GI_RESOLVE_BLOCK_DIM);

		const int32_2 r = int32_2(1, 1);

		for (int32 dy = -r.y; dy <= r.y; ++dy)
		{
			for (int32 dx = -r.x; dx <= r.x; ++dx)
			{
				const int32_2 block_tap = block_idx + int32_2(dx, dy);

				if (any(block_tap < 0) or any(block_tap >= block_extent)) { continue; }

				for (int32 i = 0; i < GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK; ++i)
				{
					const uint32 sample_id = (block_tap.x + block_tap.y * data.tile_count_w * GIBS_GI_RESOLVE_BLOCK_DIM) * GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK
										   + i;

					const gibs_gi_resolve_sample_res sample_res = gi_resolve_sample_res_arr[sample_id];

					if (sample_res.coverage == 0.h) { continue; }

					const float3  sample_irradiance = decode_r11g11b10(sample_res.irradiance_r11g11b10);
					const int32_2 sample_px			= block_tap * GIBS_GI_RESOLVE_BLOCK_SIZE
													+ int32_2(sample_res.sample_pos_lin % GIBS_GI_RESOLVE_BLOCK_SIZE, sample_res.sample_pos_lin / GIBS_GI_RESOLVE_BLOCK_SIZE);

					const float pos_rel	   = length_sq(px - sample_px);
					const float pos_weight = 1.f / (1.f + pos_rel * (1.f / GIBS_GI_RESOLVE_BLOCK_SIZE));

					const float3 sample_normal = decode_oct_snorm16(sample_res.normal_oct_snorm16);
					const float	 sample_z_lin  = sample_res.z_lin;

					const float lum_diff   = abs(px_luminance - luminance_rec709(sample_irradiance));
					const float lum_rel	   = lum_diff / (px_luminance + luminance_rec709(sample_irradiance) + epsilon_1e4);
					const float lum_weight = 1.f / (1.f + lum_rel * GIBS_GI_SCATTER_LUM_SCALE);

					const float w = calc_bilateral_weight(px_z_lin_curr, px_normal_curr, sample_z_lin, sample_normal, float2(1.f, 1.f))
								  * pos_weight
								  //* lum_weight
								  * sample_res.coverage;

					irradiance_sum += float4(sample_irradiance, 1.f) * w;
				}
			}
		}
	}
	gi_resolve_weight_buffer[px] = irradiance_sum.w;

	if (irradiance_sum.w > 0.f)
	{
		gi_resolve_buffer[px] = lerp(px_irradiance, irradiance_sum.xyz / irradiance_sum.w, 1.f / (age_curr + 1.f));
		// gi_resolve_buffer[px] = lerp(px_irradiance, irradiance_sum.xyz / irradiance_sum.w, 1.f / (16 + 1.f));

		// gi_resolve_buffer[px] = lerp(px_irradiance, irradiance_sum.xyz / irradiance_sum.w, 0.2f);
	}
	else
	{
		// todo
	}
}