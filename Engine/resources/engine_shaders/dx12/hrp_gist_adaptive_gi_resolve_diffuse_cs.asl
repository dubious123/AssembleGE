#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (thread_id >= gist::adaptive::diffuse_ray_count_total(data)) { return; }

	uint32		  type = 0u;
	const int32_2 px   = gist::adaptive::unpack_ray_entry(gist::adaptive::ray_entry_arr(data)[thread_id], type);

	assert(type == GIST_ADAPTIVE_RAY_TYPE_NEW_BORN or type == GIST_ADAPTIVE_RAY_TYPE_VARIANCE);

	structured_buffer<gist_ray_hit_result>		ray_hit_result_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gist_ray_lighting_result> ray_lighting_result_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	const uint32 id = gist::adaptive::ray_hit_diffuse_id(data, thread_id);

	float3 irradiance_curr;

	if (gist::calc_ray_irradiance(ray_hit_result_buffer[id], ray_lighting_result_buffer[id], irradiance_curr) is_false) { return; }

	gist::accumulate_gi(data, px, irradiance_curr, true);
}