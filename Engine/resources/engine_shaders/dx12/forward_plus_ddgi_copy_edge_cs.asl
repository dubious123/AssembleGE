#include "forward_plus_common.asli"

// based on Zander Majercik et al. ournal of Computer Graphics Techniques 2021
// "Scaling Probe-Based Real-Time Dynamic Global Illumination for Production"
// https://jcgt.org/published/0010/02/01/paper-lowres.pdf
wave_size(32)
[numthreads(DDGI_IRRADIANCE_RESOLUTION, 4, 1)] void
main_cs(uint32_3 group_id		 sv_group_id,
		uint32_3 group_thread_id sv_group_thread_id)


{
	const ddgi_data ddgi_data = load_ddgi_data();

	rw_texture_2d<float3> irradiance_atlas = global_resource_buffer[ddgi_data.irradiance_atlas_uav_id];
	rw_texture_2d<float2> visibility_atlas = global_resource_buffer[ddgi_data.visibility_atlas_uav_id];

	const uint32 half_local_size = DDGI_IRRADIANCE_RESOLUTION / 2;

	const uint32_2 top_left_corner_of_4_probe_irradiance = uint32_2(group_id.xy) * (2 * DDGI_IRRADIANCE_TILE_SIZE);
	const uint32_2 top_left_corner_of_4_probe_visibility = uint32_2(group_id.xy) * (2 * DDGI_VISIBILITY_TILE_SIZE);

	bool	 is_irradiance = true;
	uint32_2 texel_coord;
	uint32_2 pixel_coord_within_4_block;

	// First thread processes corners for 2x2 probe block.
	if (group_id.z == 0)
	{
		is_irradiance = (group_thread_id.x < half_local_size);

		const uint32_2 invocation_id = uint32_2(group_thread_id.x % half_local_size, group_thread_id.y);
		const uint32   probe_side	 = is_irradiance ? DDGI_IRRADIANCE_RESOLUTION : DDGI_VISIBILITY_RESOLUTION;

		texel_coord = is_irradiance ? top_left_corner_of_4_probe_irradiance : top_left_corner_of_4_probe_visibility;

		// Corner pixels
		pixel_coord_within_4_block = uint32_2(invocation_id * (probe_side + 1)) - uint32_2(probe_side * (invocation_id / 2));

		texel_coord += pixel_coord_within_4_block;
	}
	else
	{
		// Irradiance = 1-4, Visibility = 5-12
		// With 8x4 launch, need 4 blocks for 4 probes.
		const int32 irradiance_end_index   = 4 * (DDGI_IRRADIANCE_RESOLUTION / DDGI_IRRADIANCE_RESOLUTION);
		const int32 visibility_start_index = irradiance_end_index + 1;

		is_irradiance = group_id.z < uint32(visibility_start_index);

		const uint32 id				= uint32(group_thread_id.y);
		const uint32 probe_side		= is_irradiance ? DDGI_IRRADIANCE_RESOLUTION : DDGI_VISIBILITY_RESOLUTION;
		const uint32 probe_side_one = probe_side + 1;

		// Patterns for a 4 pixel border
		// Irradiance blocks (1-4) = 0110, Visibility blocks (5-13) = 01100110
		const uint32 thread_group_mod2 = (uint32(group_id.z) / 2) % 2;

		const uint32 local_thread_id = uint32(group_thread_id.x) + (is_irradiance ? 0 : DDGI_IRRADIANCE_RESOLUTION * (group_id.z % 2));

		uint32_2 offset_coord = uint32_2(
			1u + local_thread_id + thread_group_mod2 * (probe_side_one + 1u),
			id * probe_side_one - uint32(probe_side * (id / 2)));

		// dispatch.z = 1 + irradiance_edge_z + visibility_edge_z = 1 + 4 + 8 = 13
		const uint32 num_dispatch_z		 = 1 + 4 + 4 * (DDGI_VISIBILITY_RESOLUTION / DDGI_IRRADIANCE_RESOLUTION);
		const uint32 visibility_half_idx = (num_dispatch_z + visibility_start_index) / 2;

		// If we're in the second half of the visibility or irradiance threads, we're
		// processing columns, so flip the indices.
		if (group_id.z >= uint32(visibility_half_idx) or (group_id.z <= uint32(irradiance_end_index) and group_id.z > uint32(irradiance_end_index / 2)))
		{
			offset_coord = offset_coord.yx;
		}

		texel_coord = offset_coord + (is_irradiance ? top_left_corner_of_4_probe_irradiance : top_left_corner_of_4_probe_visibility);
	}

	const int32 probe_side_length = is_irradiance ? DDGI_IRRADIANCE_RESOLUTION : DDGI_VISIBILITY_RESOLUTION;
	const int32 probe_tile_size	  = is_irradiance ? DDGI_IRRADIANCE_TILE_SIZE : DDGI_VISIBILITY_TILE_SIZE;

	const int32_2 oct_coord = int32_2(
		texel_coord.x % probe_tile_size,
		texel_coord.y % probe_tile_size);

	const int32_2 probe_side_with_border = int32_2(probe_side_length + 1, probe_side_length + 1);

	// Rescale all border texels to have the correct
	// offset along their side.
	int32_2 read_offset = -2 * oct_coord + probe_side_with_border;

	// Set only the corner texels to sample from the opposite corner.
	// Note that only the corner texels will have offsets with
	// equal absolute value.
	if (abs(read_offset.x) == abs(read_offset.y))
	{
		read_offset = sign(read_offset) * probe_side_length;
	}

	// Set border (not corner) texels to have the correct offset
	// from their side into the valid texels (i.e., 1 or -1). Note that
	// the body if this IF statement differs from the one above.
	if (abs(read_offset.x) == probe_side_with_border.x)
	{
		read_offset.x = sign(read_offset.x);
	}
	if (abs(read_offset.y) == probe_side_with_border.y)
	{
		read_offset.y = sign(read_offset.y);
	}

	const int32_2 read_coord = read_offset + cast<int32_2>(texel_coord);

	if (is_irradiance)
	{
		const float3 inner			  = irradiance_atlas[read_coord];
		irradiance_atlas[texel_coord] = inner;
	}
	else
	{
		const float2 inner			  = visibility_atlas[read_coord];
		visibility_atlas[texel_coord] = inner;
	}
}