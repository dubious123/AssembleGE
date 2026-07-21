#include "hrp_common.asli"

float
sample_ao(float2 px, float3 world_pos, float3 normal)
{
	attr_branch()

	if (ao::enabled() is_false)
	{
		return 1.f;
	}

	texture_2d<float4>	 ao_buffer	  = global_resource_buffer[ao::load_data().h_ao_buffer_srv_id];
	texture_2d<uint32_2> gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float>	 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];

	const float3 ndc = world_to_ndc(view_proj, world_pos);

	const int32_2 extent = int32_2(backbuffer_size.x, backbuffer_size.y);

	const int32_2 px_00 = clamp(int32_2(floor(px)), zero<int32_2>(), extent - 1);
	const int32_2 px_01 = clamp(px_00 + int32_2(1, 0), zero<int32_2>(), extent - 1);
	const int32_2 px_10 = clamp(px_00 + int32_2(0, 1), zero<int32_2>(), extent - 1);
	const int32_2 px_11 = clamp(px_00 + int32_2(1, 1), zero<int32_2>(), extent - 1);

	const float2 f		  = frac(px);
	const float2 ratio_00 = float2(1.f - f.x, 1.f - f.y);
	const float2 ratio_01 = float2(f.x, 1.f - f.y);
	const float2 ratio_10 = float2(1.f - f.x, f.y);
	const float2 ratio_11 = float2(f.x, f.y);

	const float center_z_lin = calc_linear_z_reversed(cam_near_z, cam_far_z, ndc.z);

	const float z_lin_00 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[px_00]);
	const float z_lin_01 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[px_01]);
	const float z_lin_10 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[px_10]);
	const float z_lin_11 = calc_linear_z_reversed(cam_near_z, cam_far_z, depth_buffer[px_11]);

	const float3 normal_00 = decode_oct_snorm16(gbuffer[px_00].y);
	const float3 normal_01 = decode_oct_snorm16(gbuffer[px_01].y);
	const float3 normal_10 = decode_oct_snorm16(gbuffer[px_10].y);
	const float3 normal_11 = decode_oct_snorm16(gbuffer[px_11].y);

	float2 res	= zero<float2>();
	res		   += float2(ao_buffer[px_00].x, 1.f) * calc_bilateral_weight(center_z_lin, normal, z_lin_00, normal_00, ratio_00);
	res		   += float2(ao_buffer[px_01].x, 1.f) * calc_bilateral_weight(center_z_lin, normal, z_lin_01, normal_01, ratio_01);
	res		   += float2(ao_buffer[px_10].x, 1.f) * calc_bilateral_weight(center_z_lin, normal, z_lin_10, normal_10, ratio_10);
	res		   += float2(ao_buffer[px_11].x, 1.f) * calc_bilateral_weight(center_z_lin, normal, z_lin_11, normal_11, ratio_11);

	if (res.y > epsilon_1e4)
	{
		return res.x / res.y;
	}

	return ao_buffer[int32_2(px)].x;
}

template <uint32 ray_flag>
float3
rt_trace(float2 px, ray_desc desc, inout ray_query<ray_flag> query)
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

		attr_branch()

		if (ddgi_enabled())
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
		// else if (gibs_enabled())
		//{
		//	const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		//	const float ao = sample_ao(px, surface_data.world_pos, world_face_normal);

		//	const float3 irradiance = ao * gibs_sample_screen_irradiance(px, render_data.object_id, surface_data.world_pos, world_face_normal);
		//	const float3 gi_diffuse = surface_data.c_diffuse * irradiance * pi_inv;

		//	ambient_light += (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

		//	expand(MAX_ENV_LIGHT)

		//	for (uint32 i = 0; i < env_light_count; ++i)
		//	{
		//		ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		//	}
		//}
		else if (gibs::enabled())
		{
			const float ao = sample_ao(px, surface_data.world_pos, world_face_normal);

			const float3 irradiance = ao * gibs::sample_screen_irradiance(px, invalid_id_uint32, surface_data.world_pos, world_face_normal);

			ambient_light += calc_gi(surface_data, irradiance);
		}
		else
		{
			expand(MAX_ENV_LIGHT)

			for (uint32 i = 0; i < env_light_count; ++i)
			{
				ambient_light += calc_pbr_ibl(surface_data, load_env_light(i));
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

[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id,
		uint32 group_thread_id	  sv_group_thread_id)

{
	const aa_data data = load_aa_data();

	byte_address_buffer ray_buffer = global_resource_buffer[data.h_ray_buffer_srv_id];

	const uint32_2 scratch			= load<uint32_2>(ray_buffer, sizeof(uint16_2) * data.px_headroom);
	const uint32   opaque_rpp		= scratch.x;
	const uint32   opaque_ray_count = scratch.y;

	if (dispatch_thread_id >= opaque_ray_count)
	{
		return;
	}


	// rpp is pow of 2
	// rpp < 32

	// entry_id = tid / rpp
	const uint32 entry_id = dispatch_thread_id >> first_bit_high(opaque_rpp);

	// local_id = tid % rpp
	const uint16 local_id = cast<uint16>(dispatch_thread_id & (opaque_rpp - 1u));


	const uint16_2 px = load<uint16_2>(ray_buffer, sizeof(uint16_2) * entry_id);

	// px + 0.5 + rng( [0, 1] ) - 0.5
	const float2 screen_pos = px + random_r2_sequence(1 + local_id);

	const float3 world_far = ndc_to_world(view_proj_inv, screen_to_ndc(screen_pos, 0.f, inv_backbuffer_size));

	const float3 ray_dir = normalize(world_far - camera_pos);

	ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

	ray_desc desc;
	desc.Origin	   = camera_pos;
	desc.Direction = ray_dir;
	desc.TMin	   = 0.f;
	desc.TMax	   = float_max;

	const float3 col = rt_trace(screen_pos, desc, query);

	const float3 col_prefix = wave_prefix_sum(col);

	// if (local_id == opaque_rpp - 1)
	//{
	const float3 col_prefix_prev = group_thread_id >= opaque_rpp ? wave_read_lane_at(col_prefix, group_thread_id - opaque_rpp + 1) : zero<float3>();
	const float3 col_sum		 = col_prefix - col_prefix_prev + col;

	rw_texture_2d<float4> res_tex = global_resource_buffer[blend_buffer_uav_id];

	if (local_id == opaque_rpp - 1)
	{
		res_tex[px] = float4(max(zero<float3>(), col_sum) / opaque_rpp, 1.f - 1.f / (opaque_rpp + 1));
	}
	//}
}