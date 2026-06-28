#include "hrp_common.asli"

uint32
get_object_id(uint32 vis_packed)
{
	const uint32 render_id = vis_packed & 0x01ffffff;
	const uint32 prim_id   = (vis_packed & 0xfe000000) >> (32u - 7u);

	const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_id);

	return render_data.object_id;
}

bool
is_edge(int32_2 extent, int32_2 px)
{
	if (any(px >= extent)) { return false; }

	texture_2d<float>	 depth_buffer = global_resource_buffer[depth_buffer_texture_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[gbuffer_srv_id];

	const float	 z_depth   = depth_buffer[px];
	const float3 normal	   = decode_oct_snorm16(gbuffer[px].y);
	const uint32 object_id = get_object_id(gbuffer[px].x);

	if (z_depth == 0.f) { return false; }

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
			const float3 nbr_normal	   = decode_oct_snorm16(gbuffer[nbr_px].y);
			const uint32 nbr_object_id = get_object_id(gbuffer[nbr_px].x);

			const float3 nbr_world_pos = ndc_to_world(view_proj_inv, screen_px_to_ndc(nbr_px, nbr_z_depth, inv_backbuffer_size));
			const float	 plane_dist	   = abs(dot(nbr_world_pos - world_pos, normal));
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
	const segment_data data			= load_segment_data();
	const int32_2	   extent		= cast<int32_2>(backbuffer_size);
	const int32		   tile_count_w = ceil(extent.x, SEGMENT_TILE_SIZE);

	const int32_2 tile_id	   = group_id.xy;
	const int32	  tile_id_flat = tile_id.x + tile_id.y * tile_count_w;

	const int32_2 px = tile_id * SEGMENT_TILE_SIZE + int32_2(thread_id % SEGMENT_TILE_SIZE, thread_id / SEGMENT_TILE_SIZE);

	bool res = is_edge(extent, px);

	const uint32 wave_bitmask = wave_active_ballot(res).x;

	if (wave_is_first_lane())
	{
		rw_byte_address_buffer segment_buffer = global_resource_buffer[data.h_segment_buffer_uav_id];

		const uint32 offset = (tile_id_flat * (ceil(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 32)) + (thread_id / 32)) * sizeof(uint32);
		store(segment_buffer, offset, wave_bitmask);
	}
}
