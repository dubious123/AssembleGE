#include "hrp_common.asli"

static const float2 k_poisson_8[8] = {
	float2(-0.4706069f, -0.4427112f),
	float2(-0.9057375f, +0.3003471f),
	float2(-0.3487388f, +0.4037880f),
	float2(+0.1023042f, +0.6439373f),
	float2(+0.5699277f, +0.3513750f),
	float2(+0.2939128f, -0.1131226f),
	float2(+0.7836658f, -0.4208784f),
	float2(+0.1564120f, -0.8198990f),
};

[numthreads(16, 16, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(dispatch_thread_id.xy);

	if (any(px >= extent)) { return; }

	rw_texture_2d<float3> specular_final_buffer = global_resource_buffer[data.h_gi_resolve_specular_final_buffer_uav_id];
	texture_2d<float4>	  specular_curr_buffer	= global_resource_buffer[data.h_gi_resolve_specular_curr_buffer_srv_id];

	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float2>	 mr_buffer	  = global_resource_buffer[opaque_mr_buffer_srv_id];

	const float px_depth = depth_buffer[px];

	// sky
	if (px_depth == 0.f)
	{
		specular_final_buffer[px] = zero<float3>();
		return;
	}

	const float roughness = max(mr_buffer[px].g, brdf::ggx::roughness_min);
	if (roughness >= gist::specular_roughness_max(data))
	{
		specular_final_buffer[px] = zero<float3>();
		return;
	}

	const float4 center	  = specular_curr_buffer[px];
	const float	 hit_dist = center.a;

	const float px_z_lin  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);
	const float alpha	  = roughness * roughness;
	const float radius_px = clamp(gist::focal_px() * alpha * hit_dist / px_z_lin, 0.f, 16.f);

	if (radius_px < 0.5f)
	{
		specular_final_buffer[px] = center.rgb;
		return;
	}

	const float3 px_normal	  = decode_oct_snorm16(gbuffer[px].y);
	const float3 px_world_pos = screen_px_to_world(px, px_depth, inv_backbuffer_size, view_proj_inv);
	const float3 view		  = normalize(camera_pos - px_world_pos);
	const float	 n_dot_v	  = max(dot(px_normal, view), 0.1f);

	const float px_size_y_per_z = 2.f * tan_fov_y_half * inv_backbuffer_size.y;
	const float tolerance		= px_z_lin * px_size_y_per_z * 2.f / n_dot_v;

	const float radian = random_pcg3d(uint32_3(uint32(px.x), uint32(px.y), g::shader_hash)).x * pi_2;

	float3 color_sum = center.rgb;
	float  w_sum	 = 1.f;

	expand_all()

	for (uint32 i = 0; i < 8; ++i)
	{
		const int32_2 px_tap = clamp(px + rotate(k_poisson_8[i] * radius_px, radian), zero<int32_2>(), extent - 1);

		const float tap_depth = depth_buffer[px_tap];

		if (tap_depth == 0.f) { continue; }

		const float4 tap		   = specular_curr_buffer[px_tap];
		const float	 tap_roughness = mr_buffer[px_tap].g;
		const float3 tap_normal	   = decode_oct_snorm16(gbuffer[px_tap].y);
		const float3 tap_world_pos = screen_px_to_world(px_tap, tap_depth, inv_backbuffer_size, view_proj_inv);

		const float asym		= abs(dot(tap_world_pos - px_world_pos, px_normal));
		const float w_plane		= exp(-asym / tolerance);
		const float w_normal	= pow(max(dot(px_normal, tap_normal), 0.f), 32.f);
		const float w_roughness = saturate(1.f - abs(tap_roughness - roughness) * 8.f);
		const float w_hit_dist	= abs(tap.a - hit_dist) < 0.5f * max(hit_dist, 1.f) ? 1.f : 0.f;

		const float w = w_plane
					  * w_normal
					  * w_roughness
					  * w_hit_dist;

		color_sum += tap.xyz * w;
		w_sum	  += w;
	}

	specular_final_buffer[px] = color_sum / w_sum;
}