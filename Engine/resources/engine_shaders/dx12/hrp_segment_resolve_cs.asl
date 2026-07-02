#include "hrp_common.asli"

uint32
get_object_id(bool is_opaque, uint32 vis_packed)
{
	const uint32 render_id = vis_packed & 0x01ffffff;
	const uint32 prim_id   = (vis_packed & 0xfe000000) >> (32u - 7u);

	return is_opaque
			 ? load_opaque_meshlet_render_data(render_id).object_id
			 : load_transparent_meshlet_render_data(render_id).object_id;
}

bool
is_edge(bool is_opaque, int32_2 extent, int32_2 px)
{
	if (any(px >= extent)) { return false; }

	uint32 depth_buffer_id = is_opaque ? opaque_depth_buffer_srv_id : transparent_depth_buffer_srv_id;
	uint32 gbuffer_id	   = is_opaque ? opaque_gbuffer_srv_id : transparent_gbuffer_srv_id;

	texture_2d<float>	 depth_buffer = global_resource_buffer[depth_buffer_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[gbuffer_id];

	const float	 z_depth   = depth_buffer[px];
	const bool	 is_valid  = z_depth != 0.f;
	const float3 normal	   = decode_oct_snorm16(gbuffer[px].y);
	const uint32 object_id = is_valid ? get_object_id(is_opaque, gbuffer[px].x) : invalid_id_uint32;

	// if (z_depth == 0.f) { return false; }

	const float3 world_pos = ndc_to_world(view_proj_inv, screen_px_to_ndc(px, z_depth, inv_backbuffer_size));

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

			const float	 nbr_z_depth   = depth_buffer[nbr_px];
			const bool	 is_nbr_valid  = nbr_z_depth != 0.f;
			const float3 nbr_normal	   = decode_oct_snorm16(gbuffer[nbr_px].y);
			const uint32 nbr_object_id = is_nbr_valid ? get_object_id(is_opaque, gbuffer[nbr_px].x) : invalid_id_uint32;

			const float3 nbr_world_pos = ndc_to_world(view_proj_inv, screen_px_to_ndc(nbr_px, nbr_z_depth, inv_backbuffer_size));
			const float	 plane_dist	   = abs(dot(nbr_world_pos - world_pos, normal));

			if (is_valid is_false and is_nbr_valid is_false) { continue; }

			if (is_valid != is_nbr_valid) { return true; }

			if (plane_dist > 0.05f) { return true; }

			if (dot(normal, nbr_normal) < 0.9f) { return true; }

			if (object_id != nbr_object_id) { return true; }
		}
	}

	return false;
};

wave_size(32)
[numthreads(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 1, 1)] void
main_cs(uint32_3 group_id sv_group_id,
		uint32 thread_id  sv_group_thread_id)

{
	const segment_data data		   = load_segment_data();
	const int32_2	   extent	   = cast<int32_2>(backbuffer_size);
	const int32_2	   tile_extent = ceil(extent, SEGMENT_TILE_SIZE);

	const int32_2 tile_id	   = group_id.xy;
	const int32	  tile_id_flat = tile_id.x + tile_id.y * tile_extent.x;

	const int32_2 px = tile_id * SEGMENT_TILE_SIZE + int32_2(thread_id % SEGMENT_TILE_SIZE, thread_id / SEGMENT_TILE_SIZE);

	const bool opaque_is_edge = is_edge(true, extent, px);

	const uint32 wave_bitmask_opaque = wave_active_ballot(opaque_is_edge).x;

	const bool	 transparent_is_edge	  = is_edge(false, extent, px);
	const uint32 wave_bitmask_transparent = wave_active_ballot(transparent_is_edge).x;

	if (wave_is_first_lane())
	{
		rw_byte_address_buffer segment_buffer = global_resource_buffer[data.h_segment_buffer_uav_id];

		const uint32 word_per_tile = ceil(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 32);

		const uint32 offset = (tile_id_flat * word_per_tile + (thread_id / 32)) * sizeof(uint32);
		store(segment_buffer, offset, wave_bitmask_opaque);

		const uint32 segment_tile_count = tile_extent.x * tile_extent.y;

		store(segment_buffer, segment_tile_count * (SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE) / 8 + offset, wave_bitmask_transparent);
	}
}
