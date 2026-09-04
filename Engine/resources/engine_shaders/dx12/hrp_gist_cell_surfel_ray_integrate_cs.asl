#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	byte_array<uint32> cell_alive_arr = gist::cell::alive_id_arr_curr(data);

	const uint32 cell_surfel_count = cell_alive_arr.size();

	half   surfel_radius;
	float3 surfel_position;
	float3 surfel_normal;

	if (thread_id >= cell_surfel_count) { return; }

	const uint32 alive_id	= thread_id;
	const uint32 ray_offset = gist::cell::ray_count_prefix_rw_arr(data)[alive_id] + data.tile_count_total();
	const uint16 ray_count	= gist::cell::ray_count_rw_arr(data)[alive_id];
	const uint32 surfel_id	= cell_alive_arr[alive_id];

	if (ray_count == 0) { return; }

	rw_structured_buffer<gist_surfel_msme>		msme_buffer				   = global_resource_buffer[data.h_cell_surfel_msme_buffer_uav_id];
	rw_structured_buffer<gist_cell_surfel>		surfel_buffer			   = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];
	structured_buffer<gist_ray_hit_result>		ray_hit_result_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gist_ray_lighting_result> ray_lighting_result_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	gist_cell_surfel surfel		 = surfel_buffer[surfel_id];
	const bool		 is_new_born = surfel.is_new_born();

	float3 radiance_sum = zero<float3>();

	rw_byte_array<half>	  lum_arr = gist::cell::luminance_rw_arr(data, surfel_id);
	rw_byte_array<uint16> vis_arr = gist::cell::visibility_rw_arr(data, surfel_id);

	uint32 invalid_back_face_count = 0u;
	for (uint32 i = 0; i < ray_count; ++i)
	{
		gist_ray_hit_result		 ray_hit	  = ray_hit_result_buffer[ray_offset + i];
		gist_ray_lighting_result ray_lighting = ray_lighting_result_buffer[ray_offset + i];

		if (ray_hit.distance != float_max and ray_hit.object_render_id == invalid_id_uint32)
		{
			// opaque_ss or mask_ss back face
			++invalid_back_face_count;
			continue;
		}

		const float3 dir_local = decode_world_hemi_oct_snorm8(uint32_lower_to_uint16(ray_hit.dir_oct_snorm8));
		const uint32 idx	   = gist::calc_atlas_tile_local_idx(dir_local);
		const float	 cos_theta = dir_local.y;

		const float lum_blend_factor = is_new_born ? 1.f : 0.1f;

		const float3 radiance = decode_r11g11b10(ray_lighting.radiance_r11g11b10);

		const float contribution = cos_theta / max(epsilon_1e6, ray_hit.pdf);

		const float luminance = luminance_rec709(radiance);
		lum_arr.store(idx, cast<half>(lerp(float(lum_arr[idx]), luminance * cos_theta, lum_blend_factor)));

		// radiance, cos weight
		radiance_sum += radiance * contribution;

		const float distance = abs(ray_hit.distance);

		// visibility
		const float	 vis_blend_factor = is_new_born ? 1.f : cos_theta * 0.5f;
		const float	 dist_norm		  = saturate(distance / surfel.radius);
		const float2 chebyshev		  = float2(dist_norm, dist_norm * dist_norm);

		const uint16 chebyshev_prev_packed = vis_arr[idx];
		const float2 chebyshev_prev		   = float2(unorm8_to_float(uint32_x_to_uint8(chebyshev_prev_packed)), unorm8_to_float(uint32_y_to_uint8(chebyshev_prev_packed)));

		const float2 chebyshev_res = lerp(chebyshev_prev, chebyshev, vis_blend_factor);
		vis_arr.store(idx, uint16(float_to_unorm8(chebyshev_res.x) | (float_to_unorm8(chebyshev_res.y) << 8u)));
	}

	const bool kill = (ray_count <= 8u and invalid_back_face_count >= 1)
				   or (ray_count > 8u and invalid_back_face_count >= ray_count / 8);
	if (kill)
	{
		surfel.kill();
		surfel_buffer[surfel_id] = surfel;
		return;
	}


	radiance_sum /= (ray_count - invalid_back_face_count);

	gist_surfel_msme msme = msme_buffer[surfel_id];

	// const float t = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()));
	gist::update_msme(radiance_sum, msme /*, lerp(GIBS_MSME_SHORT_WINDOW_BLEND * 10, GIBS_MSME_SHORT_WINDOW_BLEND, t)*/);

	msme_buffer[surfel_id] = msme;

	surfel.irradiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
	surfel_buffer[surfel_id]	= surfel;
}