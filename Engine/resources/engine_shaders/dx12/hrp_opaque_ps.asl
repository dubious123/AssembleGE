#include "hrp_common.asli"

struct ps_out
{
	float4 color  sv_target_0;
	float2 motion sv_target_1;
};

ps_out
main_ps(float4 pos sv_position)
{
	const gibs_lut_data		   lut_data	 = gibs_load_gibs_lut_data();
	const texture_2d<float>	   depth_tex = global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<uint32_2> gbuffer	 = global_resource_buffer[opaque_gbuffer_srv_id];

	uint32_2	px		= uint32_2(pos.xy);
	const float z_depth = depth_tex[px];

	if (z_depth == 0.f)
	{
		discard;
		return zero<ps_out>();
	}

	const uint32 vis_packed = gbuffer[px].x;
	const uint32 render_id	= vis_packed & 0x01ffffff;
	const uint32 prim_id	= (vis_packed & 0xfe000000) >> (32u - 7u);

	const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_id);
	const material					 mat		 = load_material(render_data.material_id);
	const object_data				 obj_data	 = load_object_data(render_data.object_id);
	const mesh_header				 mesh_header = read_mesh_header<opaque_meshlet_render_data>(render_data);
	const meshlet					 mshlt		 = read_meshlet(mesh_header, render_data.meshlet_id);

	const uint32 vertex_count	 = mshlt.vertex_count_prim_count_extra & 0xffu;
	const uint32 primitive_count = (mshlt.vertex_count_prim_count_extra >> 8u) & 0xffu;

	uint32_3 meshlet_prim_idx = read_meshlet_primitive(mesh_header, mshlt, prim_id);

	// assert(prim_id < primitive_count, g::fmt_forward_plus_opaque_ps, line, prim_id, primitive_count);
	// assert(all(meshlet_prim_idx <= 64), g::fmt_forward_plus_opaque_ps, line, meshlet_prim_idx);
	// assert(all(meshlet_prim_idx <= vertex_count), g::fmt_forward_plus_opaque_ps, line, meshlet_prim_idx, vertex_count);
	// assert(vertex_count <= 64, g::fmt_forward_plus_opaque_ps, line, vertex_count);

	const vertex_fat v0 = decode_vertex(mesh_header, read_global_vertex_index(mesh_header, mshlt, meshlet_prim_idx.x));
	const vertex_fat v1 = decode_vertex(mesh_header, read_global_vertex_index(mesh_header, mshlt, meshlet_prim_idx.y));
	const vertex_fat v2 = decode_vertex(mesh_header, read_global_vertex_index(mesh_header, mshlt, meshlet_prim_idx.z));

	const float3 world_pos	  = ndc_to_world(view_proj_inv, float3(screen_to_ndc(pos.xy, inv_backbuffer_size), z_depth));
	const float3 local_pos	  = rotate_inv(obj_data.quaternion, world_pos - obj_data.pos) / obj_data.scale;
	const float3 barycentrics = calc_barycentric(local_pos, v0.pos.xyz, v1.pos.xyz, v2.pos.xyz);

	const vertex_fat v_local = interpolate_vertex_fat(v0, v1, v2, barycentrics);
	const vertex_fat v		 = transform_vertex_to_world(v_local, obj_data);

	const pbr_surface_data surface_data = calc_pbr_surface(camera_pos, mat, v);

	const float3 px_normal = decode_oct_snorm16(gbuffer[px].y);

	float3 ambient_light = float3(0, 0, 0);

	// const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));
	// const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));

	attr_branch()

	if (ddgi_enabled())
	{
		// todo need fresnel?
		// from https://google.github.io/filament/Filament.md.html
		const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		const float3 gi_diffuse	 = calc_pbr_ddgi(surface_data, px_normal);
		ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		}
	}
	else if (gibs_enabled())
	{
		const float3 f_avg = surface_data.f0 + (float3(1.f, 1.f, 1.f) - surface_data.f0) / 21;

		// const float3	   gi_diffuse		 = calc_pbr_gibs(rt_arg::init_gibs(false), invalid_id_uint32, surface_data, px_normal);
		texture_2d<float3> gi_resolve_buffer = global_resource_buffer[gibs_load_gibs_data().h_gi_resolve_full_res_buffer_srv_id];

		const float3 gi_diffuse	 = surface_data.c_diffuse * gi_resolve_buffer[px] * pi_inv;
		ambient_light			+= (1.f - f_avg) * gi_diffuse * surface_data.occlusion;

		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl_specular(surface_data, load_env_light(i)) * surface_data.occlusion;
		}
	}
	else
	{
		expand(MAX_ENV_LIGHT)

		for (uint32 i = 0; i < env_light_count; ++i)
		{
			ambient_light += calc_pbr_ibl(surface_data, load_env_light(i));
		}
	}

	float3 lighting	 = ambient_light;
	lighting		+= surface_data.emissive;

	const float3 albedo = mat.base_color_factor.rgb;

	const float3 vertex_normal = normalize(v.normal);

	const float3 world_to_cam_dir = normalize(camera_pos - v.world_pos);

	const uint32 directional_light_count = directional_light_count_and_extra & 0xff;

	for (uint32 d = 0; d < directional_light_count; ++d)
	{
		const directional_light light = load_directional_light(d);

		lighting += calc_pbr_light(surface_data, light)
				  * calc_directional_shadow_rt(light, v, px_normal);
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

	uint32 c = 0u;
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
			// wave_bit_mask			&= ~(1u << bit);
			wave_bit_mask &= (wave_bit_mask - 1u);

			if (bit_mask & (1u << bit))
			{
				const unified_light light = load_sorted_light(sorted_id);

				lighting += calc_pbr_light(surface_data, light)
						  * calc_unified_shadow_rt(light, v, px_normal);

				++c;
			}
		}
	}

	// return float4(world_face_normal, 1.f);
	// return float4(vertex_normal, 1.f);

	// return float4(c / 10.f, c / 100.f, c, 1.f);

	// motion
	const object_data obj_data_prev = load_object_prev_data(render_data.object_id);

	// todo, maybe reconstruct prev_local_pos for skinning or animated object?
	const float3 world_pos_prev = rotate(obj_data_prev.quaternion, local_pos * obj_data_prev.scale) + obj_data_prev.pos;

	const float3 ndc_prev		= world_to_ndc(view_proj_prev, world_pos_prev);
	const float2 screen_uv_prev = ndc_xy_to_screen_uv(ndc_prev.xy);

	const float2 motion = pos.xy * inv_backbuffer_size - screen_uv_prev;

	ps_out res;
	res.color  = float4(lighting, 1.f);
	res.motion = motion;
	return res;
}