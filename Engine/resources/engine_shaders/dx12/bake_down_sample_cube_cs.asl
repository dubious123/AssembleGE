#include "bake_common.asli"

[numthreads(8, 8, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id) {
	if (thread_id.x >= down_sample_cube_output_size || thread_id.y >= down_sample_cube_output_size) return;

	texture_cube<float4>		src = global_resource_buffer[down_sample_input_texture_srv_id];
	rw_texture_2d_array<float4> dst = global_resource_buffer[down_sample_output_texture_uav_id];

	const float2 face_uv = (float2(thread_id.xy) + 0.5) / float(down_sample_cube_output_size);
	const float3 dir	 = cube_face_uv_to_world_dir(face_uv, thread_id.z);

	dst[thread_id] = sample_level(src, get_linear_clamp_sampler(), dir, down_sample_mip_count);
}
