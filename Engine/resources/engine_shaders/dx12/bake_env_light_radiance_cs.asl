#include "bake_common.asli"

[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	if (thread_id.x >= env_light_radiance_size || thread_id.y >= env_light_radiance_size) { return; }

	texture_2d<float4>			equirect = global_resource_buffer[env_light_input_texture_srv_id];
	rw_texture_2d_array<float4> radiance = global_resource_buffer[env_light_radiance_texture_uav_id];

	const float2 face_uv	 = (float2(thread_id.xy) + 0.5) / float(env_light_radiance_size);	 // [0, 1]
	const float3 dir		 = cube_face_uv_to_world_dir(face_uv, thread_id.z);
	const float2 equirect_uv = float2(atan2(dir.z, dir.x) / pi_2 + 0.5, -asin(dir.y) / pi + 0.5);

	float3 color = sample(equirect, get_equirect_sampler(), equirect_uv).rgb;

	radiance[thread_id] = float4(color, 1.0);
}
