#include "hrp_common.asli"

template <uint32 ray_flag>
float3
rt_calc_opaque_color(rt_arg arg, ray_desc desc, inout ray_query<ray_flag> query)
{
	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	rt_trace_ray_inline(query, tlas, RAY_FLAG_NONE, RT_MASK_OPAQUE, desc);
	rt_proceed(query);

	const uint32 status = rt_committed_status(query);

	if (status == COMMITTED_NOTHING)
	{
		return calc_skybox_color(desc.Direction);
	}
	else if (rt_committed_triangle_front_face(query) is_false)
	{
		return float3(0, 0, 0);
	}
	else
	{
		const uint32				  rt_instance_render_data_id = rt_committed_instance_id(query);
		const uint32				  triangle_index			 = rt_committed_primitive_index(query);
		const rt_instance_render_data render_data				 = load_rt_instance_render_data(rt_instance_render_data_id);
		const material				  mat						 = load_material(render_data.material_id);
		const object_data			  obj_data					 = load_object_data(render_data.object_id);
		const mesh_header			  msh_header				 = read_mesh_header<rt_instance_render_data>(render_data);
		const uint32_3				  prim_index				 = load_rt_triangle_index(render_data, triangle_index);
		const float2				  barycentrics				 = rt_committed_triangle_barycentrics(query);

		const vertex_fat v0 = decode_vertex(msh_header, prim_index.x);
		const vertex_fat v1 = decode_vertex(msh_header, prim_index.y);
		const vertex_fat v2 = decode_vertex(msh_header, prim_index.z);

		const float3 bary_weights = float3(1.f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

		const vertex_fat v = transform_vertex_to_world(interpolate_vertex_fat(v0, v1, v2, bary_weights), obj_data);

		const pbr_surface_data surface_data = calc_pbr_surface(desc.Direction, mat, v);

		const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));

		const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));


		float3 ambient_light = float3(0, 0, 0);

		{
			// todo need fresnel?
			// from https://google.github.io/filament/Filament.md.html
			const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

			const float3 gi_diffuse	 = calc_pbr_ddgi(surface_data, world_face_normal);
			ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

			expand(MAX_ENV_LIGHT)

			for (uint32 i = 0; i < env_light_count; ++i)
			{
				ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
			}
		}

		float3 lighting = ambient_light;

		lighting += surface_data.emissive;

		const uint32 directional_light_count = directional_light_count_and_extra & 0xff;


		for (uint32 d = 0; d < directional_light_count; ++d)
		{
			const directional_light light = load_directional_light(d);

			lighting += calc_pbr_light(surface_data, light)
					  * calc_directional_shadow_rt(light, v.world_pos, world_face_normal);
		}


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
				const uint32 bit	   = first_bit_low(wave_bit_mask);
				const uint32 sorted_id = w * 32 + bit;
				// wave_bit_mask		   &= ~(1u << bit);
				wave_bit_mask &= (wave_bit_mask - 1u);

				if (bit_mask & (1u << bit))
				{
					const unified_light light = load_sorted_light(sorted_id);

					lighting += calc_pbr_light(surface_data, light)
							  * calc_unified_shadow_rt(light, v.world_pos, world_face_normal);
				}
			}
		}

		return lighting;
	}
}

ddgi_ray_result
ddgi_trace_ray(float3 pos, float3 dir /*normalized*/, float radius)
{
	float  distance;
	float3 color;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.f;
	desc.TMax	   = 3000.f;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

		color = rt_calc_opaque_color(rt_arg::init_empty(), desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			distance = 3000.f;
		}
		else if (rt_committed_triangle_front_face(query) is_false)
		{
			distance = -rt_committed_ray_t(query);
		}
		else
		{
			distance = rt_committed_ray_t(query);
		}
	}

	if (distance > 0.f)	   // not inside wall
	{
		desc.TMax = distance;

		ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

		const float4 transparent_color = rt_calc_transparent_color(rt_arg::init_empty(), desc, query);

		color = color * (1.f - transparent_color.a) + transparent_color.rgb;
	}

	ddgi_ray_result res;

	res.distance				= distance;
	res.dir_oct_snorm_and_extra = encode_oct_snorm8(dir);
	res.radiance_r11g11b10		= encode_r11g11b10(color);


	return res;
}

[numthreads(DDGI_TRACE_THREAD_PER_GROUP, 1, 1)] void
main_cs(uint32 ray_id sv_dispatch_thread_id)

{
	const ddgi_data ddgi_data = load_ddgi_data();
	if (ray_id >= ddgi_load_ray_count_total(ddgi_data) or ray_id >= DDGI_RAY_BUDGET) { return; }

	const uint32	 probe_id	  = ddgi_binary_search_probe(ddgi_data, ray_id);
	const uint32	 level		  = probe_id >> load_ddgi_ppl_log2(ddgi_data);
	const uint32	 ray_local_id = ray_id - ddgi_load_ray_offset(ddgi_data, probe_id);
	const ddgi_probe probe		  = load_ddgi_probe_srv(probe_id);

	const float3 probe_pos	  = ddgi_calc_probe_pos(ddgi_data, probe_id, level) + probe.offset;
	const float	 probe_radius = max(ddgi_data.base_probe_spacing) * (1u << level);

	// const uint32 ray_count = min(ddgi_load_ray_count(ddgi_data, probe_id), DDGI_PROBE_RAY_COUNT_NEW_BORN);
	const uint32 ray_count = ddgi_load_ray_count(ddgi_data, probe_id);

	// const float3 probe_normal = decode_oct_snorm(probe.normal_oct_snorm8);

	const uint32 idx = ray_local_id;
	const float2 xi	 = frac(random_spherical_fibonacci(idx, ray_count) + ddgi_cranley_patterson_rotation);
	// const float3 dir = normalize(probe_normal + sample_sphere_uniform(xi));

	// const float3 dir = xi.x > 0.5f
	//					 ? sample_hemisphere_cosine(xi, probe_normal)
	//					 : -sample_hemisphere_cosine(xi, probe_normal);

	const float3 dir = sample_sphere_uniform(xi);


	const ddgi_ray_result res = ddgi_trace_ray(probe_pos, dir, probe_radius);

	ddgi_store_ray_result(ddgi_data, ray_id, res);
}