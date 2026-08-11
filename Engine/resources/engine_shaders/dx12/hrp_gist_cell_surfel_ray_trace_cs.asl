#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 group_id sv_group_id,
		uint32 ray_id	sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (ray_id >= gist::cell::ray_count_total(data)) { return; }

	const gist_ray_entry ray_entry = gist::ray::load_ray_entry(data, ray_id);

	const uint32 surfel_id = ray_entry.surfel_id;

	structured_buffer<gist_cell_surfel> surfel_arr = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];

	const gist_cell_surfel surfel = surfel_arr[surfel_id];

	const float3 world_pos	   = surfel.position;
	const float3 surfel_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);

	const float ray_guide_prob = smoothstep(0.f, float(GIST_CELL_SURFEL_NEW_BORN_DELAY), float(surfel.recycle_data.frame_since_born())) * 0.5f;

	rw_structured_buffer<gist_ray_hit_result> ray_hit_buffer = global_resource_buffer[data.h_ray_hit_buffer_uav_id];

	ray_hit_buffer[data.tile_count_total() + ray_id] = gist::trace_ray_diffuse(data, world_pos, surfel_normal, random_pcg4d(uint32_4(ray_id, surfel_id, frame_index, g::shader_hash)),
																			   gist::cell::luminance_cdf_arr(data, surfel_id), ray_guide_prob, ray_id + surfel_id + frame_index + g::shader_hash);
}