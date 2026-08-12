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

	const gist_ray_hit_result hit_res = ray_hit_result_buffer[id];

	if (gist::calc_ray_radiance(hit_res, ray_lighting_result_buffer[id], radiance_curr) is_false) { return; }

	rw_texture_2d<float4> specular_buffer	  = global_resource_buffer[data.h_gi_resolve_specular_curr_buffer_uav_id];
	rw_texture_2d<uint32> specular_age_buffer = global_resource_buffer[data.h_gi_resolve_specular_age_curr_buffer_uav_id];
	texture_2d<float2>	  mr_buffer			  = global_resource_buffer[opaque_mr_buffer_srv_id];

	const float	 roughness = max(mr_buffer[px].g, brdf::ggx::roughness_min);
	const uint32 age	   = specular_age_buffer[px];

	const float blend_factor = 1.f / float(age + 1u);

	const float4 rng	= random_pcg4d(uint32_4(uint32_2(px), frame_index, g::shader_hash));
	specular_buffer[px] = round_fp16_stochastic(lerp(specular_buffer[px],
													 float4(radiance_curr, min(abs(hit_res.distance), gist::specular_hit_dist_max(data))),
													 blend_factor),
												rng);

	specular_age_buffer[px] = min(age + 1u, gist::specular_age_max(data, roughness));
}