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

	// surface
	texture_2d<float>	 depth_buffer	= global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<float2>	 mr_buffer		= global_resource_buffer[opaque_mr_buffer_srv_id];
	texture_2d<float2>	 shading_normal = global_resource_buffer[opaque_shading_normal_buffer_srv_id];
	texture_2d<uint32_2> gbuffer		= global_resource_buffer[opaque_gbuffer_srv_id];

	const float	 z_depth	   = depth_buffer[px];
	const float3 world_pos	   = screen_px_to_world(px, z_depth, inv_backbuffer_size, view_proj_inv);
	const float3 normal		   = decode_octahedral(shading_normal[px]);
	const float3 vertex_normal = decode_oct_snorm16(gbuffer[px].y);
	const float3 view		   = normalize(camera_pos - world_pos);
	const float	 roughness	   = max(mr_buffer[px].g, 0.02f);
	const float	 alpha		   = roughness * roughness;

	const float3 rand_3d = random_pcg3d(uint32_3(px.x, px.y, g::shader_hash + frame_index));

	float pdf;

	const float3x3 world_to_local = gen_world_normal_transform_t(normal);
	const float3   view_local	  = mul(world_to_local, view);
	const float3   dir_local	  = brdf::ggx::sample_vndf_dir(view_local, alpha, rand_3d.xy, pdf);

	gist_ray_hit_result res = zero<gist_ray_hit_result>();

	if (view_local.y > 0.f and dir_local.y > 0.f)
	{
		const float3 dir_world = mul(dir_local, world_to_local);
		res					   = gist::trace_ray<gist_ray_hit_result>(px.x + px.y * int32_2(backbuffer_size).x + frame_index + g::shader_hash,
																	  surface_offset(world_pos, vertex_normal), dir_world);
		res.dir_oct_snorm8	   = uint32(encode_world_hemi_oct_snorm8(dir_local)) | (uint32(encode_oct_snorm8(dir_world)) << 16u);
		res.pdf				   = pdf;
	}

	rw_structured_buffer<gist_ray_hit_result> ray_hit_buffer = global_resource_buffer[data.h_ray_hit_buffer_uav_id];

	ray_hit_buffer[gist::adaptive::ray_hit_specular_id(data, thread_id)] = res;
}