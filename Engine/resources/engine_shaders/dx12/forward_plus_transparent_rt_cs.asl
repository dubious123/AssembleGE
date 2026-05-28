#include "forward_plus_common.asli"

struct hit_data
{
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

	ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

	ray_desc desc;
	desc.Origin	   = camera_pos;
	desc.Direction = ray_dir;
	desc.TMin	   = 0.001;
	desc.TMax	   = t_max;

	hit_data hit_data_arr[MAX_RAY_HIT];
	uint32	 hit_count = 0;

	rt_trace_ray_inline(query, tlas, RAY_FLAG_NONE, RT_MASK_TRANSPARENT | RT_MASK_MASK, desc);

	while (rt_proceed(query))
	{
		float t = rt_candidate_triangle_ray_t(query);

		if (hit_count >= MAX_RAY_HIT && t >= hit_data_arr[hit_count - 1].t)
		{
			continue;
		}

		uint32 instance_id	  = rt_candidate_instance_id(query);
		uint32 triangle_index = rt_candidate_primitive_index(query);

		uint32 end = hit_count < MAX_RAY_HIT ? hit_count : hit_count - 1;
		uint32 pos = end;
		bool   dup = false;

		for (uint32 i = 0; i < end; ++i)
		{
			if (hit_data_arr[i].rt_instance_render_data_id == instance_id and hit_data_arr[i].triangle_index == triangle_index)
			{
				dup = true;
				break;
			}

			if (pos == end and t < hit_data_arr[i].t)
			{
				pos = i;
			}
		}

		if (dup) { continue; }

		for (uint32 i = end; i > pos; --i)
		{
			hit_data_arr[i] = hit_data_arr[i - 1];
		}


		hit_data_arr[pos].t							 = t;
		hit_data_arr[pos].rt_instance_render_data_id = instance_id;
		hit_data_arr[pos].triangle_index			 = triangle_index;
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
		const material				  mat		  = load_material(render_data.material_id);
		const object_data			  obj_data	  = load_object_data(render_data.object_id);
		const mesh_header			  msh_header  = read_mesh_header<rt_instance_render_data>(render_data);
		const uint32_3				  prim_index  = load_rt_triangle_index(render_data, hit.triangle_index);

		const vertex_fat v0 = decode_vertex(msh_header, prim_index.x);
		const vertex_fat v1 = decode_vertex(msh_header, prim_index.y);
		const vertex_fat v2 = decode_vertex(msh_header, prim_index.z);

		const float3 bary_weights = float3(1.f - hit.barycentrics.x - hit.barycentrics.y, hit.barycentrics.x, hit.barycentrics.y);

		const vertex_fat v = transform_vertex_to_world(interpolate_vertex_fat(v0, v1, v2, bary_weights), obj_data);

		const pbr_surface_data surface_data = calc_pbr_surface(mat, v);

		const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));

		const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));

		float3 ambient_light = float3(0, 0, 0);

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl(surface_data, load_env_light(i));
		}

		float3 lighting = ambient_light;

		lighting += surface_data.emissive;

		const float linear_depth = dot(v.world_pos - camera_pos, camera_forward);

		const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

		for (uint32 d = 0; d < directional_light_count; ++d)
		{
			const directional_light light = load_directional_light(d);

			lighting += calc_pbr_light(surface_data, light)
					  * calc_directional_shadow_rt(light, v, world_face_normal, linear_depth);
		}

		{
			const uint32_3 light_bin_axis = world_to_light_bin_axis(v.world_pos);

			const zbin_entry x_entry = load_bin_entry_x(light_bin_axis.x);
			const zbin_entry y_entry = load_bin_entry_y(light_bin_axis.y);
			const zbin_entry z_entry = load_bin_entry_z(light_bin_axis.z);

			const uint32 min_id = max(x_entry.min_idx, max(y_entry.min_idx, z_entry.min_idx));
			const uint32 max_id = min(x_entry.max_idx, min(y_entry.max_idx, z_entry.max_idx));

			const uint32 wave_min = wave_active_min(min_id);
			const uint32 wave_max = wave_active_max(max_id);

			const uint32 word_begin = wave_min / 32;
			const uint32 word_end	= wave_max / 32;

			for (uint32 w = word_begin; w <= word_end; ++w)
			{
				uint32 x_mask	= load_bin_mask_x(light_bin_axis.x, w);
				uint32 y_mask	= load_bin_mask_y(light_bin_axis.y, w);
				uint32 z_mask	= load_bin_mask_z(light_bin_axis.z, w);
				uint32 bit_mask = x_mask & y_mask & z_mask;

				uint32 wave_bit_mask = wave_active_bit_or(bit_mask);

				while (wave_bit_mask != 0)
				{
					const uint32 bit		= first_bit_low(wave_bit_mask);
					const uint32 sorted_id	= w * 32 + bit;
					wave_bit_mask		   &= ~(1u << bit);

					if (bit_mask & (1u << bit))
					{
						const unified_light light = load_sorted_light(sorted_id);

						lighting += calc_pbr_light(surface_data, light)
								  * calc_unified_shadow_rt(light, v, world_face_normal);
					}
				}
			}
		}

		const float alpha = surface_data.base_color.a;

		result.rgb += lighting * alpha * (1.f - result.a);
		result.a   += alpha * (1.f - result.a);

		if (result.a >= 1.f)
		{
			break;
		}
	}

	rw_texture_2d<float4> res_tex = global_resource_buffer[rt_transparent_buffer_uav_texture_id];

	res_tex[dispatch_thread_id.xy] = result;
}