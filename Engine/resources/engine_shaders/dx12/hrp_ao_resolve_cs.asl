#include "hrp_common.asli"

// based on Olivier Therrien et al. 2023
// "Screen Space Indirect Lighting with Visibility Bitmask"
// https://arxiv.org/pdf/2301.11376
// and XeGTAO by Filip Strugar et al.
// https://github.com/GameTechDev/XeGTAO/tree/master

#define AO_AGE_SCALE 4

[numthreads(16, 16, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const ao_data data	 = ao::load_data();
	const int32_2 px	 = int32_2(thread_id.xy);
	const int32_2 extent = cast<int32_2>(backbuffer_size);

	if (any(px >= extent)) { return; }

	texture_2d<float>	 depth_buffer			= global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer				= global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<uint32_2> opaque_geo_prev_buffer = global_resource_buffer[opaque_geo_prev_buffer_srv_id];
	texture_2d<float2>	 motion_buffer			= global_resource_buffer[motion_buffer_srv_id];

	rw_texture_2d<float> ao_res_buffer = global_resource_buffer[data.h_ao_buffer_uav_id];

	texture_2d<float>  ao_raw_prev_buffer		  = global_resource_buffer[data.h_ao_raw_prev_buffer_srv_id];
	texture_2d<float2> ao_bent_normal_prev_buffer = global_resource_buffer[data.h_ao_bent_normal_prev_buffer_srv_id];
	texture_2d<uint32> ao_age_prev_buffer		  = global_resource_buffer[data.h_ao_age_prev_buffer_srv_id];

	rw_texture_2d<float>  ao_raw_curr_buffer		 = global_resource_buffer[data.h_ao_raw_curr_buffer_uav_id];
	rw_texture_2d<float2> ao_bent_normal_curr_buffer = global_resource_buffer[data.h_ao_bent_normal_curr_buffer_uav_id];
	rw_texture_2d<uint32> ao_age_curr_buffer		 = global_resource_buffer[data.h_ao_age_curr_buffer_uav_id];

	const float z_depth = depth_buffer[px];

	if (z_depth <= 0.f)
	{
		ao_res_buffer[px]			   = 1.f;
		ao_raw_curr_buffer[px]		   = 1.f;
		ao_bent_normal_curr_buffer[px] = encode_octahedral(float3(0.f, 0.f, 1.f));
		ao_age_curr_buffer[px]		   = 0u;
		return;
	}

	const float3 px_normal	 = decode_oct_snorm16(gbuffer[px].y);
	const float3 view_normal = normalize(mul(cast<float3x3>(view), px_normal));
	const float	 px_z_lin	 = calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth);

	const float3 rng	   = random_pcg3d(uint32_3(px.x, px.y, frame_index));
	const float	 px_world  = px_world_size(z_depth, tan_fov_y_half, backbuffer_size.y);
	const float	 px_radius = min(data.radius / px_world, data.max_px_radius);	 // (m) / (m / px)

	// todo, add proj_inv to optimize this?
	const float3 view_pos	  = ndc_to_view(view, view_proj_inv, screen_px_to_ndc(px, z_depth, inv_backbuffer_size));
	const float	 view_dist	  = length(view_pos);
	const float3 view_forward = -view_pos / view_dist;

	const uint32 slice_idx = frame_index % data.slice_count();
	const float	 phi	   = (slice_idx + rng.x) / float(data.slice_count()) * pi;

	const float cos_phi = cos(phi);
	const float sin_phi = sin(phi);

	const float3 view_right	  = reject_norm(float3(cos_phi, sin_phi, 0), view_forward);
	const float3 slice_normal = cross(view_forward, view_right);

	const float3 view_normal_proj = reject_norm(view_normal, slice_normal);

	bool l_valid = true;
	bool r_valid = true;

	uint32 occlusion_mask = zero<uint32>();

	for (uint32 j = 0; j < data.offset_count() and (l_valid or r_valid); ++j)
	{
		const float	  jitter	   = random_r1_sequence(slice_idx * data.offset_count() + j, rng.y);
		const float	  offset_dist  = (pow((j + jitter) / data.offset_count(), 2) + (1.f / px_radius)) * px_radius;
		const float2  offset_float = min(offset_dist, data.max_px_radius) * float2(cos_phi, -sin_phi);
		const int32_2 offset_px	   = int32_2(round(offset_float));

		if (l_valid)
		{
			const int32_2 sample_pos_px_l = px - offset_px;

			l_valid &= all(sample_pos_px_l >= 0) and all(sample_pos_px_l < extent);

			if (l_valid)
			{
				const float	 sample_z_depth_l  = depth_buffer[sample_pos_px_l];
				const float3 sample_view_pos_l = ndc_to_view(view, view_proj_inv, screen_px_to_ndc(sample_pos_px_l, sample_z_depth_l, inv_backbuffer_size));
				const float3 view_rel		   = sample_view_pos_l - view_pos;
				const float	 rel_dist		   = length(view_rel);
				const float3 rel_dir		   = view_rel / rel_dist;
				const float	 side			   = dot(rel_dir, view_right) >= 0 ? 1.f : -1.f;

				const float cos_theta = dot(rel_dir, view_normal);

				if (rel_dist > epsilon_1e4 and cos_theta > 0)
				{
					const float front_theta		= side * fast_acos(cos_theta);
					const float thickness_theta = fast_atan_pos(data.thickness / rel_dist);
					const float back_theta		= front_theta + side * thickness_theta;

					const float front_theta_norm = saturate(front_theta * pi_inv + 0.5f);
					const float back_theta_norm	 = saturate(back_theta * pi_inv + 0.5f);

					const uint32 front_bin = cast<uint32>(clamp(front_theta_norm * 32, 0, 31));
					const uint32 back_bin  = cast<uint32>(clamp(back_theta_norm * 32, 0, 31));

					const uint32 start_bin = min(front_bin, back_bin);
					const uint32 end_bin   = max(front_bin, back_bin);

					const uint32 mask = ((1u << end_bin) - 1u) & ~((1u << start_bin) - 1u);

					occlusion_mask |= mask;
				}
			}
		}

		if (r_valid)
		{
			const int32_2 sample_pos_px_r = px + offset_px;

			r_valid &= all(sample_pos_px_r >= 0) and all(sample_pos_px_r < extent);

			if (r_valid)
			{
				const float	 sample_z_depth_r  = depth_buffer[sample_pos_px_r];
				const float3 sample_view_pos_r = ndc_to_view(view, view_proj_inv, screen_px_to_ndc(sample_pos_px_r, sample_z_depth_r, inv_backbuffer_size));
				const float3 view_rel		   = sample_view_pos_r - view_pos;
				const float	 rel_dist		   = length(view_rel);
				const float3 rel_dir		   = view_rel / rel_dist;
				const float	 side			   = dot(rel_dir, view_right) >= 0 ? 1.f : -1.f;

				const float cos_theta = dot(rel_dir, view_normal);

				if (rel_dist > epsilon_1e4 and cos_theta > 0)
				{
					const float front_theta		= side * fast_acos(cos_theta);
					const float thickness_theta = fast_atan_pos(data.thickness / rel_dist);
					const float back_theta		= front_theta + side * thickness_theta;

					const float front_theta_norm = saturate(front_theta * pi_inv + 0.5f);
					const float back_theta_norm	 = saturate(back_theta * pi_inv + 0.5f);

					const uint32 front_bin = cast<uint32>(clamp(front_theta_norm * 32, 0, 31));
					const uint32 back_bin  = cast<uint32>(clamp(back_theta_norm * 32, 0, 31));

					const uint32 start_bin = min(front_bin, back_bin);
					const uint32 end_bin   = max(front_bin, back_bin);

					const uint32 mask = ((1u << end_bin) - 1u) & ~((1u << start_bin) - 1u);

					occlusion_mask |= mask;
				}
			}
		}
	}


	const float3 slice_forward = cross(slice_normal, view_normal_proj);

	float3 bent_normal_sum = zero<float3>();

	for (uint32 visibility_mask = ~occlusion_mask; visibility_mask > 0; visibility_mask &= (visibility_mask - 1u))
	{
		const uint32 idx = first_bit_low(visibility_mask);

		const float theta = ((idx + 0.5f) / 32.f - 0.5f) * pi;

		bent_normal_sum += view_normal_proj * cos(theta) + slice_forward * sin(theta);
	}

	const float3 bent_normal_view = any(bent_normal_sum != 0.f) ? normalize(bent_normal_sum) : view_normal;

	// ao_curr

	const float3 ao_bent_normal_curr = mul(bent_normal_view, cast<float3x3>(view));
	float		 ao_curr_raw		 = countbits(~occlusion_mask) / 32.f;

	float  ao_res_raw		  = ao_curr_raw;
	float3 ao_bent_normal_res = ao_bent_normal_curr;
	uint32 ao_age_res		  = 0u;

	// ao prev

	const float2 motion = denoise::sample_motion(motion_buffer, depth_buffer, px, extent);

	const denoise::reproject_taps rp_taps = denoise::reproject_taps::init(opaque_geo_prev_buffer, px, extent, motion, z_depth, px_normal, px_z_lin);

	if (rp_taps.is_prev_valid)
	{
		float  ao_raw_prev_sum		   = 0.f;
		float3 ao_bent_normal_prev_sum = zero<float3>();
		float  ao_age_prev_sum		   = 0.f;

		expand_all()

		for (uint32 i = 0; i < 4; ++i)
		{
			if (rp_taps.w[i] <= 0.f) { continue; }

			ao_raw_prev_sum			+= ao_raw_prev_buffer[rp_taps.px[i]] * rp_taps.w[i];
			ao_bent_normal_prev_sum += decode_octahedral(ao_bent_normal_prev_buffer[rp_taps.px[i]]) * rp_taps.w[i];
			ao_age_prev_sum			+= float(ao_age_prev_buffer[rp_taps.px[i]]) * rp_taps.w[i];
		}

		if (rp_taps.w_sum > 0.f)
		{
			const float	 ao_prev_raw		 = ao_raw_prev_sum / rp_taps.w_sum;
			const float3 ao_bent_normal_prev = normalize(ao_bent_normal_prev_sum);

			const float	 ao_age_prev_avg = ao_age_prev_sum / rp_taps.w_sum + 1.f;
			const float	 ao_age_prev_f	 = rp_taps.quality < 1.f ? ao_age_prev_avg * sqrt(rp_taps.quality) : ao_age_prev_avg;
			const uint32 ao_age_prev_i	 = clamp(uint32(round(ao_age_prev_f)), 1u, uint32(data.slice_count() * AO_AGE_SCALE));

			const float blend_factor = 1.f / (ao_age_prev_i + 1u);

			ao_res_raw = lerp(ao_prev_raw, ao_curr_raw, blend_factor);

			const float3 ao_bent_blend = lerp(ao_prev_raw * ao_bent_normal_prev, ao_curr_raw * ao_bent_normal_curr, blend_factor);

			ao_bent_normal_res = any(ao_bent_blend != 0.f) ? normalize(ao_bent_blend) : ao_bent_normal_curr;

			ao_age_res = ao_age_prev_i;
		}
	}

	float ao_res = ao_res_raw;

	ao_res = pow(ao_res, data.power);
	ao_res = lerp(1.f, ao_res, data.intensity);
	ao_res = lerp(ao_res, 1.f, smoothstep(data.fade_distance - data.fade_range, data.fade_distance, view_dist));

	ao_res_buffer[px]			   = ao_res;
	ao_raw_curr_buffer[px]		   = ao_res_raw;
	ao_bent_normal_curr_buffer[px] = encode_octahedral(ao_bent_normal_res);
	ao_age_curr_buffer[px]		   = ao_age_res;
}