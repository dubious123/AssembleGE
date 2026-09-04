#include "hrp_common.asli"

// wave_size(AGE_WAVE_SIZE)
//[numthreads(16, 16, 1)] void
// main_cs(uint32_3 thread_id sv_dispatch_thread_id)
//
//{
//	const int32_2 extent = cast<int32_2>(backbuffer_size);
//	const int32_2 px	 = int32_2(thread_id.xy);
//
//	if (any(thread_id.xy >= extent)) { return; }
//
//	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
//	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
//
//	rw_texture_2d<float4> base_color_buffer		= global_resource_buffer[opaque_base_color_buffer_uav_id];
//	rw_texture_2d<float2> mr_buffer				= global_resource_buffer[opaque_mr_buffer_uav_id];
//	rw_texture_2d<float2> shading_normal_buffer = global_resource_buffer[opaque_shading_normal_buffer_uav_id];
//	rw_texture_2d<float3> emissive_buffer		= global_resource_buffer[opaque_emissive_buffer_uav_id];
//
//	const float z_depth = depth_buffer[px];
//
//	if (z_depth == 0.f) { return; }
//
//	const mesh::surface_point_data surface_point = mesh::calc_surface_point(gbuffer, px, z_depth);
//
//	const pbr_surface_data pbr_surface = calc_pbr_surface<true>(normalize(surface_point.v.world_pos - camera_pos), surface_point.mat, surface_point.v);
//
//	base_color_buffer[px]	  = float4(linear_to_srgb(pbr_surface.base_color.rgb), pbr_surface.occlusion);
//	mr_buffer[px]			  = float2(pbr_surface.metallic, pbr_surface.roughness);
//	shading_normal_buffer[px] = encode_octahedral(pbr_surface.normal);
//	emissive_buffer[px]		  = pbr_surface.emissive;
// }

wave_size(AGE_WAVE_SIZE)
[numthreads(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 1, 1)] void
main_cs(uint32_3 group_id	   sv_group_id,
		uint32 group_thread_id sv_group_thread_id)

{
	const int32_2 extent	  = cast<int32_2>(backbuffer_size);
	const int32_2 tile_extent = ceil(extent, SEGMENT_TILE_SIZE);

	const int32_2 tile_id	   = group_id.xy;
	const int32	  tile_id_flat = tile_id.x + tile_id.y * tile_extent.x;

	const int32_2 px = tile_id * SEGMENT_TILE_SIZE + int32_2(group_thread_id % SEGMENT_TILE_SIZE, group_thread_id / SEGMENT_TILE_SIZE);

	bool is_valid = true;

	is_valid &= all(px < extent);

	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];

	rw_texture_2d<float4> base_color_buffer		= global_resource_buffer[opaque_base_color_buffer_uav_id];
	rw_texture_2d<float2> mr_buffer				= global_resource_buffer[opaque_mr_buffer_uav_id];
	rw_texture_2d<float2> shading_normal_buffer = global_resource_buffer[opaque_shading_normal_buffer_uav_id];
	rw_texture_2d<float3> emissive_buffer		= global_resource_buffer[opaque_emissive_buffer_uav_id];

	const float z_depth	 = is_valid ? depth_buffer[px] : 0.f;
	is_valid			&= z_depth != 0.f;

	bool is_fade = false;
	if (is_valid)
	{
		const mesh::surface_point_data surface_point = mesh::calc_surface_point(gbuffer, px, z_depth, is_fade);

		const pbr_surface_data pbr_surface = calc_pbr_surface<true>(normalize(surface_point.v.world_pos - camera_pos), surface_point.mat, surface_point.v);

		base_color_buffer[px]	  = float4(linear_to_srgb(pbr_surface.base_color.rgb), pbr_surface.occlusion);
		mr_buffer[px]			  = float2(pbr_surface.metallic, pbr_surface.roughness);
		shading_normal_buffer[px] = encode_octahedral(pbr_surface.normal);
		emissive_buffer[px]		  = pbr_surface.emissive;
	}


	// todo, disable edge detection?
	// attr_branch()
	// if (aa::enabled() and aa::load_data().opaque_aa_enabled() is_false) { return; }

	static_assert(AGE_WAVE_SIZE == 32);
	const uint32 wave_bitmask_opaque_is_fade = wave_active_ballot(is_fade).x;
	if (wave_is_first_lane())
	{
		rw_byte_address_buffer segment_buffer = global_resource_buffer[load_segment_data().h_segment_buffer_uav_id];
		const uint32		   word_per_tile  = ceil(SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE, 32);
		const uint32		   offset		  = tile_extent.x * tile_extent.y * (SEGMENT_TILE_SIZE * SEGMENT_TILE_SIZE) / 8	   // 8 bit per byte
												  * 2
											  + (tile_id_flat * word_per_tile + (group_thread_id / 32)) * sizeof(uint32);
		store(segment_buffer, offset, wave_bitmask_opaque_is_fade);


		// const uint32 segment_tile_count = tile_extent.x * tile_extent.y;

		// store(segment_buffer, segment_tile_count * (16 * 16) / 8 + offset, wave_bitmask_transparent);
	}
}
