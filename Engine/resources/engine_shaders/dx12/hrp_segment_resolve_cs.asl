#include "hrp_common.asli"

uint32
get_object_render_id(uint32 vis_packed)
{
	const uint32 render_id = unpack_vis_mshlt_render_id(vis_packed);
	return meshlet_render_data_buffer[render_id].object_render_id;
}

bool
is_fade(rw_byte_address_buffer segment_buf, int32_2 tile_extent, int32_2 px)
{
	const int32_2 tile_id		= px / SEGMENT_TILE_SIZE;
	const int32	  tile_id_flat	= tile_id.x + tile_id.y * tile_extent.x;
	const int32_2 local			= px - tile_id * SEGMENT_TILE_SIZE;
	const uint32  bit_idx		= local.x + local.y * SEGMENT_TILE_SIZE;
	const uint32  word_per_tile = ceil(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 32u);
	const uint32  offset		= 2 * tile_extent.x * tile_extent.y * word_per_tile * sizeof(uint32)
								+ (tile_id_flat * word_per_tile + bit_idx / 32) * sizeof(uint32);
	return (load<uint32>(segment_buf, offset) >> (bit_idx % 32)) & 1u;
}

bool
is_edge(bool is_opaque, int32_2 extent, int32_2 px)
{
	if (any(px >= extent)) { return false; }

	const segment_data	   data			  = load_segment_data();
	rw_byte_address_buffer segment_buffer = global_resource_buffer[data.h_segment_buffer_uav_id];
	const int32_2		   tile_extent	  = ceil(extent, SEGMENT_TILE_SIZE);

	const uint32 depth_buffer_id = is_opaque ? opaque_depth_buffer_srv_id : transparent_depth_buffer_srv_id;
	const uint32 gbuffer_id		 = is_opaque ? opaque_gbuffer_srv_id : transparent_gbuffer_srv_id;

	texture_2d<float>	 depth_buffer = global_resource_buffer[depth_buffer_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[gbuffer_id];

	const float	 z_depth		  = depth_buffer[px];
	const bool	 is_valid		  = z_depth != 0.f;
	const float3 normal			  = decode_oct_snorm16(gbuffer[px].y);
	const uint32 object_render_id = is_valid ? get_object_render_id(gbuffer[px].x) : invalid_id_uint32;

	// if (z_depth == 0.f) { return false; }

	const float3 world_pos = screen_px_to_world(px, z_depth, inv_backbuffer_size, view_proj_inv);

	const float	 px_size_y_per_z = 2.f * tan_fov_y_half / backbuffer_size.y;
	const float3 view_dir		 = normalize(camera_pos - world_pos);
	const float	 n_dot_v		 = max(dot(normal, view_dir), 0.1f);
	const float	 tolerance		 = calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth) * px_size_y_per_z * data.edge_plane_dist_tolerance_px / n_dot_v;

	expand_all()
	for (int32 y = -1; y <= 1; ++y)
	{
		const int32 sample_y = px.y + y;
		if (sample_y < 0 or sample_y >= extent.y) { continue; }

		expand_all()
		for (int32 x = -1; x <= 1; ++x)
		{
			const int32 sample_x = px.x + x;
			if (sample_x < 0 or sample_x >= extent.x) { continue; }
			if (x == 0 and y == 0) { continue; }

			const int32_2 nbr_px = int32_2(sample_x, sample_y);

			// center is not fade, if nbr is_fade, ignore
			if (is_opaque and is_fade(segment_buffer, tile_extent, nbr_px)) { continue; }

			const float	 nbr_z_depth   = depth_buffer[nbr_px];
			const bool	 is_nbr_valid  = nbr_z_depth != 0.f;
			const float3 nbr_normal	   = decode_oct_snorm16(gbuffer[nbr_px].y);
			const uint32 nbr_object_id = is_nbr_valid ? get_object_render_id(gbuffer[nbr_px].x) : invalid_id_uint32;
			const float3 nbr_world_pos = ndc_to_world(view_proj_inv, screen_px_to_ndc(nbr_px, nbr_z_depth, inv_backbuffer_size));
			const float	 plane_dist	   = abs(dot(nbr_world_pos - world_pos, normal));

			if (is_valid is_false and is_nbr_valid is_false) { continue; }

			if (is_valid != is_nbr_valid) { return true; }

			if (plane_dist > tolerance) { return true; }

			if (dot(normal, nbr_normal) < data.edge_normal_threshold) { return true; }

			if (object_render_id != nbr_object_id) { return true; }
		}
	}

	return false;
};

wave_size(AGE_WAVE_SIZE)
[numthreads(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 1, 1)] void
main_cs(uint32_3 group_id sv_group_id,
		uint32 thread_id  sv_group_thread_id)

{
	const segment_data	   data			  = load_segment_data();
	rw_byte_address_buffer segment_buffer = global_resource_buffer[data.h_segment_buffer_uav_id];

	const int32_2 extent	  = cast<int32_2>(backbuffer_size);
	const int32_2 tile_extent = ceil(extent, SEGMENT_TILE_SIZE);

	const int32_2 tile_id	   = group_id.xy;
	const int32	  tile_id_flat = tile_id.x + tile_id.y * tile_extent.x;

	const int32_2 px = tile_id * SEGMENT_TILE_SIZE + int32_2(thread_id % SEGMENT_TILE_SIZE, thread_id / SEGMENT_TILE_SIZE);

	const bool is_center_fade = is_fade(segment_buffer, tile_extent, px);

	const bool opaque_is_edge = is_center_fade is_false and is_edge(true, extent, px);

	const uint32 wave_bitmask_opaque = wave_active_ballot(opaque_is_edge).x;

	const bool	 transparent_is_edge	  = is_edge(false, extent, px);
	const uint32 wave_bitmask_transparent = wave_active_ballot(transparent_is_edge).x;

	if (wave_is_first_lane())
	{
		const uint32 word_per_tile = ceil(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 32);

		const uint32 offset = (tile_id_flat * word_per_tile + (thread_id / 32)) * sizeof(uint32);
		store(segment_buffer, offset, wave_bitmask_opaque);

		const uint32 segment_tile_count = tile_extent.x * tile_extent.y;

		store(segment_buffer, segment_tile_count * (SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE) / 8 /*byte_size*/ + offset, wave_bitmask_transparent);
	}
}
