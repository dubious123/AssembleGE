#include "hrp_common.asli"

groupshared hrp_gbuffer_as_out as_out;

[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_as(
	uint32_3 dispatch_thread_id sv_dispatch_thread_id,
	uint32_3 group_id			sv_group_id)

{
	const uint32 visible_mask = wave_active_ballot(mesh::is_meshlet_visible<mesh::raster_mode_kind::opaque, false>(dispatch_thread_id.x)).x;

	if (wave_is_first_lane())
	{
		as_out.meshlet_group_idx  = group_id.x;
		as_out.meshlet_alive_mask = visible_mask;
	}

	dispatch_mesh(countbits(visible_mask), 1, 1, as_out);
}