#include "hrp_common.asli"

[numthreads(16, 16, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	const int32_2 extent = int32_2(backbuffer_size);
	const int32_2 px	 = int32_2(dispatch_thread_id.xy);

	if (any(px >= extent)) { return; }

	rw_texture_2d<uint32> gi_resolve_age_curr_buffer = global_resource_buffer[data.h_gi_resolve_age_curr_buffer_uav_id];

	const uint32 age = gi_resolve_age_curr_buffer[px];

	structured_buffer<gist_ray_hit_result>		ray_hit_result_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gist_ray_lighting_result> ray_lighting_result_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	const uint32  tile_size		  = data.tile_size();
	const int32_2 tile_idx_center = px / int32(tile_size);

	const uint32 center_tile_id = gist::tile::calc_id(data, uint32_2(tile_idx_center));
	const bool	 is_round_robin = all(px == gist::tile::calc_px(data, uint32_2(tile_idx_center), center_tile_id));

	float3 irradiance_curr;

	if (age >= 4)
	{
		if (is_round_robin is_false) { return; }

		if (gist::calc_ray_irradiance(ray_hit_result_buffer[center_tile_id], ray_lighting_result_buffer[center_tile_id], irradiance_curr) is_false) { return; }
	}
	else
	{
		texture_2d<float>	 depth_buffer	= global_resource_buffer[opaque_depth_buffer_srv_id];
		texture_2d<uint32_2> opaque_gbuffer = global_resource_buffer[opaque_gbuffer_srv_id];

		const float px_depth = depth_buffer[px];

		if (px_depth == 0.f) { return; }

		const float3 px_normal	  = decode_oct_snorm16(opaque_gbuffer[px].y);
		const float3 px_world_pos = screen_px_to_world(px, px_depth, inv_backbuffer_size, view_proj_inv);
		const float	 px_z_lin	  = calc_linear_z_reversed(cam_near_z, cam_far_z, px_depth);

		const float tile_world_size = float(tile_size) * px_z_lin * 2.f * tan_fov_y_half * inv_backbuffer_size.y;

		float  w_sum		 = 0.f;
		uint32 sel_tile_id	 = invalid_id_uint32;
		uint32 candidate_idx = 0u;

		for (int32 y = -3; y <= 3; ++y)
		{
			for (int32 x = -3; x <= 3; ++x)
			{
				const int32_2 tile_tap = tile_idx_center + int32_2(x, y);

				if (any(tile_tap < zero<int32_2>()) or any(tile_tap >= int32_2(data.tile_count_w, data.tile_count_h))) { continue; }

				const uint32			  tile_id = gist::tile::calc_id(data, uint32_2(tile_tap));
				const gist_ray_hit_result ray_hit = ray_hit_result_buffer[tile_id];

				if (gist::is_valid_ray_hit(ray_hit) is_false) { continue; }

				const int32_2 tap_px = gist::tile::calc_px(data, uint32_2(tile_tap), tile_id);

				const float	 tap_depth	   = depth_buffer[tap_px];
				const float3 tap_normal	   = decode_oct_snorm16(opaque_gbuffer[tap_px].y);
				const float3 tap_world_pos = screen_px_to_world(tap_px, tap_depth, inv_backbuffer_size, view_proj_inv);

				const float dist_w = all(tap_px == px)
									   ? 1.f
									   : saturate(abs(ray_hit.distance) / (tile_world_size * 3.f));

				const float pos_w = 1.f / (1.f + length_sq(float2(px - tap_px)) / float(tile_size * tile_size));

				const float w = calc_bilateral_weight(px_world_pos, px_normal, px_z_lin, tap_world_pos, tap_normal, tan_fov_y_half, inv_backbuffer_size)
							  * dist_w
							  * pos_w;

				++candidate_idx;

				w_sum += w;

				const float rng = random_pcg3d(uint32_3(uint32(px.x), uint32(px.y), frame_index * 16u + candidate_idx)).x;

				if (rng * w_sum <= w) { sel_tile_id = tile_id; }
			}
		}

		if (sel_tile_id == invalid_id_uint32 or w_sum <= 0.05f) { return; }

		if (gist::calc_ray_irradiance(ray_hit_result_buffer[sel_tile_id], ray_lighting_result_buffer[sel_tile_id], irradiance_curr) is_false) { return; }
	}

	gist::accumulate_gi(data, px, irradiance_curr, is_round_robin);
}