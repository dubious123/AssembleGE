#include "hrp_common.asli"

// based on Olivier Therrien et al. 2023
// "Screen Space Indirect Lighting with Visibility Bitmask"
// https://arxiv.org/pdf/2301.11376
// and XeGTAO by Filip Strugar et al.
// https://github.com/GameTechDev/XeGTAO/tree/master

[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	const ao_data data	 = ao::load_data();
	const int32_2 px	 = thread_id.xy;
	const int32_2 extent = cast<int32_2>(backbuffer_size);

	if (any(px >= extent)) { return; }

	texture_2d<float>	  depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2>  gbuffer	   = global_resource_buffer[opaque_gbuffer_srv_id];
	rw_texture_2d<float4> ao_buffer	   = global_resource_buffer[data.h_ao_buffer_uav_id];

	const float3 px_normal	 = decode_oct_snorm16(gbuffer[px].y);
	const float3 view_normal = normalize(mul(cast<float3x3>(view), px_normal));
	const float	 z_depth	 = depth_buffer[px];

	if (z_depth <= 0.f) { return; }

	const float3 rng	   = random_pcg3d(uint32_3(px.x, px.y, frame_index));
	const float	 px_world  = px_world_size(z_depth, tan_fov_y_half, backbuffer_size.y);
	const float	 px_radius = min(data.radius / px_world, data.max_px_radius);	 // (m) / (m / px)

	// todo, add proj_inv to optimize this?
	const float3 view_pos	  = ndc_to_view(view, view_proj_inv, screen_px_to_ndc(px, z_depth, inv_backbuffer_size));
	const float	 view_dist	  = length(view_pos);
	const float3 view_forward = -view_pos / view_dist;

	float2 ao_sum		   = zero<float2>();
	float3 bent_normal_sum = zero<float3>();

	for (uint32 i = 0; i < data.slice_count(); ++i)
	{
		const float phi		= (i + rng.x) / data.slice_count() * pi;
		const float cos_phi = cos(phi);
		const float sin_phi = sin(phi);

		const float3 view_right	  = reject_norm(float3(cos_phi, sin_phi, 0), view_forward);
		const float3 slice_normal = cross(view_forward, view_right);

		const float3 view_normal_proj = reject_norm(view_normal, slice_normal);
		// const float	 view_normal_angle = fast_acos(dot(view_forward, view_normal));

		bool l_valid = true;
		bool r_valid = true;

		uint32 occlusion_mask = zero<uint32>();
		for (uint32 j = 0; j < data.offset_count() and (l_valid or r_valid); ++j)
		{
			const float	  jitter	   = random_r1_sequence(i * data.offset_count() + j, rng.y);
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

		ao_sum += float2(countbits(~occlusion_mask) / 32.f, 1.f);

		const float3 slice_forward = cross(slice_normal, view_normal_proj);

		for (uint32 visibility_mask = ~occlusion_mask; visibility_mask > 0; visibility_mask &= (visibility_mask - 1u))
		{
			const uint32 idx = first_bit_low(visibility_mask);

			const float theta = ((idx + 0.5f) / 32.f - 0.5f) * pi;

			bent_normal_sum += view_normal_proj * cos(theta) + slice_forward * sin(theta);
		}
	}

	float ao_res = ao_sum.y > 0.f ? ao_sum.x / ao_sum.y : 1.f;

	ao_res = pow(ao_res, data.power);
	ao_res = lerp(1.f, ao_res, data.intensity);
	ao_res = lerp(ao_res, 1.f, smoothstep(data.fade_distance - data.fade_range, data.fade_distance, view_dist));

	const float3 bent_normal_view = any(bent_normal_sum != 0.f) ? normalize(bent_normal_sum) : view_normal;

	const float3 bent_normal_world = mul(bent_normal_view, cast<float3x3>(view));	 // mul(transpose(view), v)

	ao_buffer[px] = float4(ao_res, (encode_octahedral(normalize(bent_normal_world)) + 1.f) * 0.5f, 1.f);
}
