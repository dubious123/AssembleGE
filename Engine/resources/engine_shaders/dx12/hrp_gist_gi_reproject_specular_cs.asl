#include "hrp_common.asli"

// based on Nathan Reed 2015
// https://computergraphics.stackexchange.com/questions/1718/what-is-the-simplest-way-to-compute-principal-curvature-for-a-mesh-triangle
float
calc_curvature(texture_2d<uint32_2> gbuffer, texture_2d<float> depth_buffer,
			   int32_2 px, int32_2 extent, float3 world_pos, float3 normal)
{
	const float px_size_y = length(world_pos - camera_pos) * 2.f * tan_fov_y_half * inv_backbuffer_size.y;

	float  curvature_sum = 0.f;
	uint32 count		 = 0u;

	expand_all()

	for (uint32 i = 0; i < 2; ++i)
	{
		const int32_2 px_tap = min(px + (i == 0 ? int32_2(1, 0) : int32_2(0, 1)), extent - 1);

		const float z_depth = depth_buffer[px_tap];

		// sky
		if (z_depth == 0.f) { continue; }

		const float3 dp	   = screen_px_to_world(px_tap, z_depth, inv_backbuffer_size, view_proj_inv) - world_pos;
		const float	 dp_sq = dot(dp, dp);

		if (dp_sq <= 0.f) { continue; }
		if (dp_sq > px_size_y * px_size_y * 16.f) { continue; }

		const float3 dn = decode_oct_snorm16(gbuffer[px_tap].y) - normal;

		// curvature = dot(dn, dp) / |dp|^2
		curvature_sum += dot(dn, dp) / dp_sq;
		++count;
	}

	return count > 0u ? curvature_sum / float(count) : 0.f;
};

[numthreads(16, 16, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(dispatch_thread_id.xy);

	if (any(px >= extent)) { return; }

	rw_texture_2d<float4> specular_curr_buffer	   = global_resource_buffer[data.h_gi_resolve_specular_curr_buffer_uav_id];
	texture_2d<float4>	  specular_prev_buffer	   = global_resource_buffer[data.h_gi_resolve_specular_prev_buffer_srv_id];
	rw_texture_2d<uint32> specular_age_curr_buffer = global_resource_buffer[data.h_gi_resolve_specular_age_curr_buffer_uav_id];
	texture_2d<uint32>	  specular_age_prev_buffer = global_resource_buffer[data.h_gi_resolve_specular_age_prev_buffer_srv_id];

	texture_2d<float>	 depth_buffer			= global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer				= global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float2>	 mr_buffer				= global_resource_buffer[opaque_mr_buffer_srv_id];
	texture_2d<uint32_2> opaque_geo_prev_buffer = global_resource_buffer[opaque_geo_prev_buffer_srv_id];
	texture_2d<float2>	 motion_buffer			= global_resource_buffer[motion_buffer_srv_id];

	const float px_depth = depth_buffer[px];

	if (px_depth == 0.f)
	{
		specular_curr_buffer[px]	 = zero<float4>();
		specular_age_curr_buffer[px] = 0u;
		return;
	}

	const float roughness = max(mr_buffer[px].g, brdf::ggx::roughness_min);
	if (roughness >= gist::specular_roughness_max(data))
	{
		specular_curr_buffer[px]	 = zero<float4>();
		specular_age_curr_buffer[px] = 0u;
		return;
	}


	const float3 px_normal	  = decode_oct_snorm16(gbuffer[px].y);
	const float3 px_world_pos = screen_px_to_world(px, px_depth, inv_backbuffer_size, view_proj_inv);
	const float	 px_z_lin	  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

	const float hit_dist = specular_prev_buffer[px].a;


	const float2 motion	 = denoise::sample_motion(motion_buffer, depth_buffer, px, extent);
	const float2 uv_curr = (float2(px) + 0.5f) * inv_backbuffer_size;

	// uv_surface with cam movement and obj movement (cam + obj)
	const float2 uv_surface = uv_curr - motion;

	const float3 view_ray = normalize(px_world_pos - camera_pos);

	const float kappa = clamp(calc_curvature(gbuffer, depth_buffer, px, extent, px_world_pos, px_normal), 0.f, 16.f);

	// roughness up => hit_dist down
	const float3 virtual_pos = px_world_pos + view_ray * (hit_dist / (1.f + 2.f * hit_dist * kappa)) * (1.f - roughness / gist::specular_roughness_max(data));
	// uv_virtual when object did not move (cam only)
	const float2 uv_virtual = ndc_xy_to_screen_uv(world_to_ndc(view_proj_prev, virtual_pos).xy);

	// uv_surface when object did not move (cam only)
	const float2 uv_cam_surface = ndc_xy_to_screen_uv(world_to_ndc(view_proj_prev, px_world_pos).xy);

	// uv_prev = uv_virtual( cam ) + ( uv_surface( cam + obj ) - uv_surface ( cam ) )
	const float2 uv_prev		 = uv_virtual + (uv_surface - uv_cam_surface);
	const float2 motion_specular = uv_curr - uv_prev;

	const denoise::reproject_taps rp_taps = denoise::reproject_taps::init(opaque_geo_prev_buffer, px, extent, motion_specular, px_world_pos, px_normal, px_z_lin);

	if (rp_taps.is_prev_valid and rp_taps.w_sum > 0.f)
	{
		float4 specular_sum = zero<float4>();
		float  age_sum		= 0.f;
		float  w_sum		= 0.f;

		expand_all()

		for (uint32 i = 0; i < 4; ++i)
		{
			if (rp_taps.w[i] <= 0.f) { continue; }

			const float4 tap = specular_prev_buffer[rp_taps.px[i]];
			const float	 w	 = rp_taps.w[i] * (abs(tap.a - hit_dist) < 0.5f * max(hit_dist, 1.f) ? 1.f : 0.f);

			specular_sum += tap * w;
			age_sum		 += float(specular_age_prev_buffer[rp_taps.px[i]]) * w;
			w_sum		 += w;
		}

		if (w_sum > 0.f)
		{
			const float4 rng = random_pcg4d(uint32_4(uint32_2(px), frame_index, g::shader_hash));

			specular_curr_buffer[px] = round_fp16_stochastic(specular_sum / w_sum, rng);

			const float age_avg = age_sum / w_sum;
			const float age_res = rp_taps.quality < 1.f
									? age_avg * sqrt(rp_taps.quality)
									: age_avg;

			specular_age_curr_buffer[px] = min(uint32(round(age_res)), gist::specular_age_max(data, roughness));
			return;
		}
	}

	specular_curr_buffer[px]	 = float4(0.f, 0.f, 0.f, hit_dist);
	specular_age_curr_buffer[px] = 0u;
}