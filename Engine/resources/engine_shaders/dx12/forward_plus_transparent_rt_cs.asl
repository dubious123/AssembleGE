#include "forward_plus_common.asli"

static const uint32 MAX_RAY_HIT = 8;

struct hit_data
{
	float4 color;
	uint32 rt_instance_render_data_id;
	uint32 triangle_index;
	float2 barycentrics;
	float  t;
};

[numthreads(8, 8, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	if (dispatch_thread_id.x >= (uint32)backbuffer_size.x || dispatch_thread_id.y >= (uint32)backbuffer_size.y)
	{
		return;
	}

	const texture_2d<float> depth_tex = global_resource_buffer[depth_buffer_texture_id];
	const float				z_depth	  = load(depth_tex, dispatch_thread_id.x, dispatch_thread_id.y, 0);

	const float2 ndc = screen_to_ndc(float2(dispatch_thread_id.x + .5f, dispatch_thread_id.y + .5f), inv_backbuffer_size);

	float4 world_far  = mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	world_far.xyz	 /= world_far.w;

	const float3 ray_dir = normalize(world_far.xyz - camera_pos);
	const float	 t_max	 = length(world_far.xyz - camera_pos);

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	ray_query<RAY_FLAG_NONE> query;

	ray_desc desc;
	desc.Origin	   = camera_pos;
	desc.Direction = ray_dir;
	desc.TMin	   = 0.001;
	desc.TMax	   = t_max;

	hit_data hit_data_arr[MAX_RAY_HIT];
	uint32	 hit_count = 0;

	rt_trace_ray_inline(query, tlas, RAY_FLAG_NONE, RT_MASK_TRANSPARENT, desc);

	static const uint32 MAX_RAY_ITERATIONS = 128;
	uint32				it				   = 0;
	while (rt_proceed(query))
	{
		float  t	 = rt_candidate_triangle_ray_t(query);
		float4 color = float4(0, 1, 0, 0.1);	// todo material

		if (color.a <= 0.01) continue;

		if (++it >= MAX_RAY_ITERATIONS)
		{
			break;
		}

		if (hit_count >= MAX_RAY_HIT && t >= hit_data_arr[hit_count - 1].t)
		{
			continue;
		}

		uint32 end = hit_count < MAX_RAY_HIT ? hit_count : hit_count - 1;
		uint32 pos = end;

		for (uint32 i = 0; i < pos; ++i)
		{
			if (t < hit_data_arr[i].t)
			{
				pos = i;
				break;
			}
		}

		for (uint32 i = end; i > pos; --i)
		{
			hit_data_arr[i] = hit_data_arr[i - 1];
		}


		hit_data_arr[pos].t							 = t;
		hit_data_arr[pos].color						 = color;
		hit_data_arr[pos].rt_instance_render_data_id = rt_candidate_instance_id(query);
		hit_data_arr[pos].triangle_index			 = rt_candidate_primitive_index(query) * 3;
		hit_data_arr[pos].barycentrics				 = rt_candidate_triangle_barycentrics(query);

		if (hit_count < MAX_RAY_HIT)
		{
			hit_count++;
		}
	}

	float4 result = float4(0, 0, 0, 0);
	for (int32 i = hit_count - 1; i >= 0; --i)
	{
		const hit_data hit = hit_data_arr[i];

		const rt_instance_render_data render_data = load_rt_instance_render_data(hit.rt_instance_render_data_id);
		const object_data			  obj_data	  = load_object_data(render_data.object_id);
		const mesh_header			  msh_header  = read_mesh_header(render_data.mesh_byte_offset);
		const uint32_3				  prim_index  = load_rt_triangle_index(render_data, hit.triangle_index);

		const vertex_encoded v_encoded_0 = read_vertex_encoded(msh_header, prim_index.x);
		const vertex_encoded v_encoded_1 = read_vertex_encoded(msh_header, prim_index.y);
		const vertex_encoded v_encoded_2 = read_vertex_encoded(msh_header, prim_index.z);

		const vertex_decoded v_decoded_0 = decode_vertex<vertex_decoded>(v_encoded_0, msh_header.aabb_min, msh_header.aabb_size);
		const vertex_decoded v_decoded_1 = decode_vertex<vertex_decoded>(v_encoded_1, msh_header.aabb_min, msh_header.aabb_size);
		const vertex_decoded v_decoded_2 = decode_vertex<vertex_decoded>(v_encoded_2, msh_header.aabb_min, msh_header.aabb_size);

		const float3 bary_weights = float3(1.f - hit.barycentrics.x - hit.barycentrics.y,
										   hit.barycentrics.x,
										   hit.barycentrics.y);

		const float3 local_pos = v_decoded_0.pos.xyz * bary_weights.x
							   + v_decoded_1.pos.xyz * bary_weights.y
							   + v_decoded_2.pos.xyz * bary_weights.z;

		const float3 local_normal = v_decoded_0.normal * bary_weights.x
								  + v_decoded_1.normal * bary_weights.y
								  + v_decoded_2.normal * bary_weights.z;

		const float4 quaternion = decode_quaternion(obj_data.quaternion);
		const float3 scale		= cast<float3>(obj_data.scale);
		const float3 pos		= obj_data.pos;

		const float3 world_normal = normalize(rotate(local_normal / scale, quaternion));
		const float3 world_pos	  = rotate(local_pos * scale, quaternion) + pos;


		float4 color = hit_data_arr[i].color;
		result.rgb	 = lerp(result.rgb, color.rgb, color.a);
		result.a	 = result.a + color.a * (1.f - result.a);
	}

	rw_texture_2d<float4> res_tex = global_resource_buffer[rt_transparent_buffer_uav_texture_id];

	res_tex[dispatch_thread_id.xy] = result;
}