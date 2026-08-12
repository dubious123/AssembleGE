#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (thread_id >= gist::adaptive::specular_ray_count_total(data)) { return; }

	uint32		  type = 0u;
	const int32_2 px   = gist::adaptive::unpack_ray_entry(gist::adaptive::ray_entry_arr(data)[gist::adaptive::load_ray_entry_cap(data).x + thread_id], type);

	assert(type == GIST_ADAPTIVE_RAY_TYPE_SPECULAR);

	structured_buffer<gist_ray_hit_result>		ray_hit_result_buffer	   = global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gist_ray_lighting_result> ray_lighting_result_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	const uint32 id = gist::adaptive::ray_hit_specular_id(data, thread_id);

	float3 radiance_curr;

	if (gist::calc_ray_radiance(ray_hit_result_buffer[id], ray_lighting_result_buffer[id], radiance_curr) is_false) { return; }

	rw_texture_2d<float3> specular_buffer = global_resource_buffer[data.h_gi_resolve_specular_buffer_uav_id];

	const float3 rng	= random_pcg3d(uint32_3(uint32_2(px), g::shader_hash + frame_index));
	specular_buffer[px] = round_fp16_stochastic(lerp(specular_buffer[px], radiance_curr, 0.2f /*tune*/), rng);
}