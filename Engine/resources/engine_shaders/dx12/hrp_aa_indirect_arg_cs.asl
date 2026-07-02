#include "hrp_common.asli"

[numthreads(1, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const aa_data data = load_aa_data();

	rw_byte_address_buffer indirect_arg_buf = global_resource_buffer[data.h_indirect_arg_uav_id];

	{
		const uint32 opaque_edge_px_count = load<uint32>(indirect_arg_buf, 0);
		const uint32 opaque_ray_px_count  = min(data.px_headroom, opaque_edge_px_count);

		// opaque_ray_budget : edge_px_cap * opaque_rpp
		uint32 opaque_rpp = cast<uint32>(data.opaque_aa_ray_budget / float(opaque_ray_px_count));
		opaque_rpp		  = min(opaque_rpp, data.ray_per_px & 0xff);

		opaque_rpp = max(opaque_rpp, 1);
		// assert(opaque_rpp > 0)
		opaque_rpp = 1u << first_bit_high(opaque_rpp);

		const uint32 opaque_ray_count = opaque_ray_px_count * opaque_rpp;

		rw_byte_address_buffer ray_entry_buf = global_resource_buffer[data.h_ray_buffer_uav_id];

		store(ray_entry_buf, sizeof(uint16_2) * data.px_headroom + 0 * sizeof(uint32_2), uint32_2(opaque_rpp, opaque_ray_count));
		store(indirect_arg_buf, 0, uint32_3(ceil(opaque_ray_count, 32u), 1, 1));
	}

	{
		const uint32 transparent_edge_px_count = load<uint32>(indirect_arg_buf, sizeof(uint32_3));
		const uint32 transparent_ray_px_count  = min(data.px_headroom, transparent_edge_px_count);

		// transparent_ray_budget : edge_px_cap * transparent_rpp
		uint32 transparent_rpp = cast<uint32>(data.transparent_aa_ray_budget / float(transparent_ray_px_count));
		transparent_rpp		   = min(transparent_rpp, data.ray_per_px & 0xff);

		transparent_rpp = max(transparent_rpp, 1);
		// assert(transparent_rpp > 0)
		transparent_rpp = 1u << first_bit_high(transparent_rpp);

		const uint32 transparent_ray_count = transparent_ray_px_count * transparent_rpp;

		rw_byte_address_buffer ray_entry_buf = global_resource_buffer[data.h_ray_buffer_uav_id];

		store(ray_entry_buf, sizeof(uint16_2) * data.px_headroom + 1 * sizeof(uint32_2), uint32_2(transparent_rpp, transparent_ray_count));
		store(indirect_arg_buf, sizeof(uint32_3), uint32_3(ceil(transparent_ray_count, 32u), 1, 1));
	}
}