#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> tile_alive_arr = gibs::tile::alive_id_arr_curr(data);
	byte_array<uint32> cell_alive_arr = gibs::cell::alive_id_arr_curr(data);

	const uint32 tile_surfel_count = tile_alive_arr.size();
	const uint32 cell_surfel_count = cell_alive_arr.size();

	const bool is_tile = thread_id < tile_surfel_count;

	uint32 ray_offset;
	uint32 ray_count;
	uint32 surfel_id;

	gibs_surfel_msme msme;

	half   surfel_radius;
	float3 surfel_position;
	float3 surfel_normal;

	bool is_new_born;
	if (is_tile)
	{
		const uint32 alive_id = thread_id;
		ray_offset			  = gibs::tile::ray_count_prefix_rw_arr(data)[alive_id];
		ray_count			  = gibs::tile::ray_count_rw_arr(data)[alive_id];
		surfel_id			  = tile_alive_arr[alive_id];

		rw_structured_buffer<gibs_surfel_msme> msme_buffer	 = global_resource_buffer[data.h_tile_surfel_msme_buffer_uav_id];
		rw_structured_buffer<gibs_tile_surfel> surfel_buffer = global_resource_buffer[data.h_tile_surfel_buffer_uav_id];

		msme = msme_buffer[surfel_id];

		gibs_tile_surfel surfel = surfel_buffer[surfel_id];
		surfel_radius			= surfel.radius;
		surfel_position			= surfel.position;
		surfel_normal			= decode_oct_snorm16(surfel.normal_oct_snorm16);
		is_new_born				= surfel.is_new_born();
	}
	else if (thread_id - tile_surfel_count < cell_surfel_count)
	{
		const uint32 alive_id = thread_id - tile_surfel_count;
		ray_offset			  = gibs::cell::ray_count_prefix_rw_arr(data)[alive_id] + gibs::tile::ray_count_total(data);
		ray_count			  = gibs::cell::ray_count_rw_arr(data)[alive_id];
		surfel_id			  = cell_alive_arr[alive_id];

		rw_structured_buffer<gibs_surfel_msme> msme_buffer	 = global_resource_buffer[data.h_cell_surfel_msme_buffer_uav_id];
		rw_structured_buffer<gibs_cell_surfel> surfel_buffer = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];
		msme												 = msme_buffer[surfel_id];

		gibs_cell_surfel surfel = surfel_buffer[surfel_id];
		surfel_radius			= surfel.radius;
		surfel_position			= surfel.position;
		surfel_normal			= decode_oct_snorm16(surfel.normal_oct_snorm16);
		is_new_born				= surfel.is_new_born();
	}
	else
	{
		return;
	}

	float3 radiance_sum = zero<float3>();

	if (ray_count == 0) { return; }

	rw_byte_array<half>	  lum_arr = gibs::load_lum_rw_arr(data, surfel_id, is_tile);
	rw_byte_array<uint16> vis_arr = gibs::load_vis_rw_arr(data, surfel_id, is_tile);

	structured_buffer<gibs_ray_hit_result>		ray_hit_result_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gibs_ray_lighting_result> ray_lighting_result_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	uint32 opaque_back_face_count = 0u;
	for (uint32 i = 0; i < ray_count; ++i)
	{
		gibs_ray_hit_result		 ray_hit	  = ray_hit_result_buffer[ray_offset + i];
		gibs_ray_lighting_result ray_lighting = ray_lighting_result_buffer[ray_offset + i];

		if (ray_hit.distance < 0.f and ray_hit.object_id == invalid_id_uint32)
		{
			// opaque back face
			++opaque_back_face_count;
			continue;
		}

		const float3 dir_local = decode_world_hemi_oct_snorm8(uint32_lower_to_uint16(ray_hit.dir_oct_snorm8));
		const uint32 idx	   = gibs::calc_atlas_tile_local_idx(dir_local);
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
		const float	 dist_norm		  = saturate(distance / surfel_radius);
		const float2 chebyshev		  = float2(dist_norm, dist_norm * dist_norm);

		const uint16 chebyshev_prev_packed = vis_arr[idx];
		const float2 chebyshev_prev		   = float2(unorm8_to_float(uint32_x_to_uint8(chebyshev_prev_packed)), unorm8_to_float(uint32_y_to_uint8(chebyshev_prev_packed)));

		const float2 chebyshev_res = lerp(chebyshev_prev, chebyshev, vis_blend_factor);
		vis_arr.store(idx, uint16(float_to_unorm8(chebyshev_res.x) | (float_to_unorm8(chebyshev_res.y) << 8u)));
	}

	if (opaque_back_face_count == ray_count)
	{
		// todo, kill?
		return;
	}

	radiance_sum /= (ray_count - opaque_back_face_count);

	// const float t = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()));
	gibs::update_msme(radiance_sum, msme /*, lerp(GIBS_MSME_SHORT_WINDOW_BLEND * 10, GIBS_MSME_SHORT_WINDOW_BLEND, t)*/);

	if (is_tile)
	{
		rw_structured_buffer<gibs_surfel_msme> msme_buffer	 = global_resource_buffer[data.h_tile_surfel_msme_buffer_uav_id];
		rw_structured_buffer<gibs_tile_surfel> surfel_buffer = global_resource_buffer[data.h_tile_surfel_buffer_uav_id];

		msme_buffer[surfel_id] = msme;

		gibs_tile_surfel surfel		= surfel_buffer[surfel_id];
		surfel.irradiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
		surfel_buffer[surfel_id]	= surfel;
	}
	else
	{
		rw_structured_buffer<gibs_surfel_msme> msme_buffer	 = global_resource_buffer[data.h_cell_surfel_msme_buffer_uav_id];
		rw_structured_buffer<gibs_cell_surfel> surfel_buffer = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];

		msme_buffer[surfel_id] = msme;

		gibs_cell_surfel surfel		= surfel_buffer[surfel_id];
		surfel.irradiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
		surfel_buffer[surfel_id]	= surfel;
	}
}