#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data					  data			 = gibs_load_gibs_data();
	const byte_array<uint32>		  alive_id_arr	 = gibs_load_alive_arr_curr(data);
	const rw_byte_array<uint32>		  ray_offset_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	const rw_byte_array<uint32>		  ray_count_arr	 = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	const byte_array<gibs_ray_result> ray_res_arr	 = gibs_load_ray_result_arr(data);
	const rw_byte_array<surfel>		  surfel_arr	 = gibs_load_surfel_rw_arr(data);
	const rw_byte_array<surfel_msme>  msme_arr		 = gibs_load_surfel_msme_rw_arr(data);
	const uint32					  alive_count	 = alive_id_arr.size();

	attr_branch()

	if (alive_count == 0)
	{
		return;
	}

	if (thread_id >= alive_count)
	{
		return;
	}

	const uint32 alive_id  = thread_id;
	const uint32 surfel_id = alive_id_arr[thread_id];

	const uint32 ray_offset = ray_offset_arr[alive_id];
	const uint32 ray_count	= ray_count_arr[alive_id];

	rw_byte_array<half>	  lum_arr = gibs_load_lum_rw_arr(data, surfel_id);
	rw_byte_array<uint16> vis_arr = gibs_load_surfel_visibility_rw_arr(data, surfel_id);

	float3		radiance_sum = (float3)0;
	surfel		surfel		 = surfel_arr[surfel_id];
	surfel_msme msme		 = msme_arr[surfel_id];

	const bool is_new_born = surfel.is_new_born();

	// assert(gibs_load_alive_surfel_id_stack_curr(data)[surfel.alive_idx] == surfel_id, g::fmt_gibs_gi_resolve, line);

	if (ray_count == 0) { return; }
	if (surfel.radius == 0.f) { return; }

	for (uint32 i = 0; i < ray_count; ++i)
	{
		gibs_ray_result ray_res = ray_res_arr[ray_offset + i];

		const bool	 is_back_face = ray_res.distance < 0.f;
		const float3 dir_local	  = decode_world_hemi_oct_snorm8(cast<uint16>(ray_res.dir_oct_snorm8 & 0xffffu));
		const uint32 idx		  = gibs_calc_tile_local_idx(dir_local);
		const float	 cos_theta	  = dir_local.y;

		if (is_back_face is_false)
		{
			// luminance
			const float lum_blend_factor = is_new_born ? 1.f : 0.1f;

			const float3 radiance = decode_r11g11b10(ray_res.radiance_r11g11b10);


			// assert(dir_local.y >= 0.f,g::fmt_forward_plus_gibs_ray_integrate_cs);

			const float contribution = cos_theta / max(epsilon_1e6, ray_res.pdf);

			const float luminance = luminance_rec709(radiance);
			lum_arr.store(idx, cast<half>(lerp(float(lum_arr[idx]), luminance * cos_theta, lum_blend_factor)));

			// radiance, cos weight
			radiance_sum += radiance * contribution;
		}

		ray_res.distance = max(0.f, ray_res.distance);

		// visibility
		const float	 vis_blend_factor = is_new_born ? 1.f : cos_theta * 0.5f;
		const float	 dist_norm		  = saturate(ray_res.distance / (surfel.radius /* * GIBS_SURFEL_OUTER_RADIUS_FACTOR*/));
		const float2 chebyshev		  = float2(dist_norm, dist_norm * dist_norm);

		const uint16 chebyshev_prev_packed = vis_arr[idx];
		const float2 chebyshev_prev		   = float2(unorm8_to_float(uint32_x_to_uint8(chebyshev_prev_packed)), unorm8_to_float(uint32_y_to_uint8(chebyshev_prev_packed)));

		const float2 chebyshev_res = lerp(chebyshev_prev, chebyshev, vis_blend_factor);
		vis_arr.store(idx, uint16(float_to_unorm8(chebyshev_res.x) | (float_to_unorm8(chebyshev_res.y) << 8u)));
	}

	radiance_sum /= ray_count;

	// gibs_update_msme(radiance_sum, msme, (4.f - 3.f * smoothstep(0, GIBS_RADIANCE_CACHE_DELAY, surfel.frame_since_born())) * GIBS_MSME_SHORT_WINDOW_BLEND);
	gibs_update_msme(radiance_sum, msme, surfel.frame_since_born() < GIBS_RADIANCE_CACHE_DELAY ? GIBS_MSME_SHORT_WINDOW_BLEND * 10 : GIBS_MSME_SHORT_WINDOW_BLEND);
	surfel.radiance_r11g11b10 = encode_r11g11b10(msme.mean_long);

	// if (is_new_born)
	//{
	//	surfel.radiance_r11g11b10 = encode_r11g11b10(radiance_sum);
	// }
	// else if (surfel.frame_since_born() < GIBS_RADIANCE_CACHE_DELAY)
	//{
	//	surfel.radiance_r11g11b10 = encode_r11g11b10((decode_r11g11b10(surfel.radiance_r11g11b10) + radiance_sum) * 0.5f);
	// }
	// else
	//{
	//	surfel.radiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
	// }


	assert(is_nan(radiance_sum) is_false /*and msme.inconsistency <= 1.f*/, g::fmt_forward_plus_gibs_ray_integrate_cs, line, radiance_sum, msme.inconsistency);

	surfel_arr.store(surfel_id, surfel);
	msme_arr.store(surfel_id, msme);
}