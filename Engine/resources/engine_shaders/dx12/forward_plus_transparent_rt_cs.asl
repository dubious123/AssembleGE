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

	const float2 screen_pos = float2(dispatch_thread_id.x + .5f, dispatch_thread_id.y + .5f);

	const float2 ndc = screen_to_ndc(screen_pos, inv_backbuffer_size);

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
		float4 color = float4(0.8, 0.8, 0.8, 0.3);	  // todo material

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
		hit_data_arr[pos].triangle_index			 = rt_candidate_primitive_index(query);
		hit_data_arr[pos].barycentrics				 = rt_candidate_triangle_barycentrics(query);

		if (hit_count < MAX_RAY_HIT)
		{
			hit_count++;
		}
	}

	float4 result = float4(0, 0, 0, 0);
	for (int32 i = 0; i < hit_count; ++i)
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

		const float3 local_vertex_normal = v_decoded_0.normal * bary_weights.x
										 + v_decoded_1.normal * bary_weights.y
										 + v_decoded_2.normal * bary_weights.z;

		const float3 local_face_normal = normalize(cross(v_decoded_1.pos.xyz - v_decoded_0.pos.xyz, v_decoded_2.pos.xyz - v_decoded_0.pos.xyz));

		const float4 quaternion = decode_quaternion(obj_data.quaternion);
		const float3 scale		= cast<float3>(obj_data.scale);
		const float3 pos		= obj_data.pos;

		const float3 world_vertex_normal = normalize(rotate(local_vertex_normal / scale, quaternion));
		const float3 world_face_normal	 = normalize(rotate(local_face_normal / scale, quaternion));
		const float3 world_pos			 = rotate(local_pos * scale, quaternion) + pos;

		const float3 world_to_cam_dir = normalize(camera_pos - world_pos);

		const float3 ambient_light = float3(0.03, 0.03, 0.03);
		float3		 lighting	   = ambient_light;

		const float linear_depth = dot(world_pos - camera_pos, camera_forward);

		const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

		for (uint32 d = 0; d < directional_light_count; ++d)
		{
			const directional_light light = load_directional_light(d);

			lighting += calc_directional_light(light, world_vertex_normal, world_to_cam_dir)
					  * calc_directional_shadow(light, world_pos, world_face_normal, linear_depth);
		}

		{
			const uint32 tile_x	 = uint32(dispatch_thread_id.x) / LIGHT_TILE_SIZE;
			const uint32 tile_y	 = uint32(dispatch_thread_id.y) / LIGHT_TILE_SIZE;
			const uint32 tile_id = tile_x + tile_y * light_tile_count_x;

			const uint32 bin = clamp(depth_to_bin(linear_depth), 0, Z_SLICE_COUNT - 1);

			const uint32 z_min = load_zbin_entry(bin).min_idx;
			const uint32 z_max = load_zbin_entry(bin).max_idx;

			const uint32 wave_z_min = wave_active_min(z_min);
			const uint32 wave_z_max = wave_active_max(z_max);

			const uint32 word_begin = wave_z_min / 32;
			const uint32 word_end	= wave_z_max / 32;


			for (uint32 w = word_begin; w <= word_end; ++w)
			{
				uint32 bit_mask		 = load_tile_mask(tile_id, w);
				uint32 wave_bit_mask = wave_active_bit_or(bit_mask);

				while (wave_bit_mask != 0)
				{
					const uint32 bit		= first_bit_low(wave_bit_mask);
					const uint32 sorted_id	= w * 32 + bit;
					wave_bit_mask		   &= ~(1u << bit);

					// if (bit_mask & (1u << bit))
					{
						const unified_light light = load_sorted_light(sorted_id);

						lighting += calc_unified_light(light, world_pos, world_vertex_normal, world_to_cam_dir)
								  * calc_unified_shadow(light, world_pos, world_face_normal);
					}
				}
			}
		}

		float4 color  = hit_data_arr[i].color * float4(lighting.rgb, 1.f);
		result.rgb	 += color.rgb * color.a * (1.f - result.a);
		result.a	 += color.a * (1.f - result.a);

		if (result.a >= 1.f)
		{
			break;
		}
	}

	rw_texture_2d<float4> res_tex = global_resource_buffer[rt_transparent_buffer_uav_texture_id];

	res_tex[dispatch_thread_id.xy] = result;
}