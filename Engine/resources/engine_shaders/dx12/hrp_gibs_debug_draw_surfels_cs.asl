#include "hrp_common.asli"

struct debug_query_data
{
	// cell
	uint32 max_contribution_cell_surfel_id;
	uint32 oldest_cell_surfel_id;
	uint32 cell_surfel_count;
	float  cell_surfel_coverage;

	// probe
	uint32 max_contribution_probe_id;
	uint32 oldest_probe_id;

	float3 cell_irradiance_near;
	float3 cell_irradiance_far;
	float3 cell_irradiance;
	uint32 probe_count;

	float probe_near_coverage;
	float probe_far_coverage;
	float cell_surfel_near_sh_coverage;
	float cell_surfel_far_sh_coverage;

	float max_contribution_probe_visibility;
	float oldest_probe_visibility;

	// tile
	uint32 max_contribution_tile_surfel_id;
	uint32 oldest_tile_surfel_id;
	float3 tile_irradiance;
	float  tile_surfel_coverage;
	uint32 tile_surfel_count;

	static debug_query_data
	init()
	{
		debug_query_data res				= zero<debug_query_data>();
		res.max_contribution_cell_surfel_id = invalid_id_uint32;
		res.oldest_cell_surfel_id			= invalid_id_uint32;
		res.max_contribution_probe_id		= invalid_id_uint32;
		res.oldest_probe_id					= invalid_id_uint32;
		res.max_contribution_tile_surfel_id = invalid_id_uint32;
		res.oldest_tile_surfel_id			= invalid_id_uint32;
		return res;
	}
};

void
get_tile_surfel(const gibs_data data, const gibs_lut_data lut_data, int32_2 px, float3 world_pos, float3 normal, inout debug_query_data res)
{
	const int32_2				 tile_idx	= px / GIBS_GI_RESOLVE_TILE_SIZE;
	const uint32				 tile_id	= gibs::tile::calc_id(data, tile_idx);
	const gibs_tile_surfel_entry tile_entry = gibs::tile::surfel_entry_arr(data)[tile_id];

	texture_2d<float3>					gi_resolve_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_srv_id];
	structured_buffer<gibs_tile_surfel> surfel_buffer	  = global_resource_buffer[data.h_tile_surfel_buffer_srv_id];

	float  coverage					  = 0.f;
	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;
	uint32 surfel_id_oldest			  = invalid_id_uint32;
	uint32 surfel_age_max			  = 0u;

	float4 irradiance_sum = zero<float4>();

	for (uint32 i = 0; i < min(512, tile_entry.surfel_count); ++i)
	{
		const uint32 surfel_id = gibs::tile::tile_to_surfel_id_arr(data)[tile_entry.offset + i];

		const gibs_tile_surfel surfel	  = surfel_buffer[surfel_id];
		const uint32		   surfel_age = surfel.recycle_data.frame_since_born();

		const float contribution = gibs::calc_tile_surfel_contribution<false>(data, surfel, world_pos, normal);

		const float3 surfel_irradiance = decode_r11g11b10(surfel.irradiance_r11g11b10);

		if (contribution == 0.f) { continue; }

		const float visibility = gibs::calc_surfel_visibility<true, gibs_tile_surfel>(data, surfel_id, surfel, world_pos);

		const float contribution_vis = contribution * visibility;

		coverage += contribution_vis;

		irradiance_sum += float4(surfel_irradiance, 1.f)
						* contribution_vis
						* smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel_age));

		if (contribution_vis > 0.f)
		{
			if (max_contribution < contribution_vis)
			{
				max_contribution		   = contribution_vis;
				max_contribution_surfel_id = surfel_id;
			}

			if (surfel_age >= surfel_age_max)
			{
				if (surfel_age == surfel_age_max and surfel_id_oldest < surfel_id)
				{
				}
				else
				{
					surfel_id_oldest = surfel_id;
				}

				surfel_age_max = surfel_age;
			}
		}
	}

	if (irradiance_sum.w < 0.1f)
	{
		const float4 fallback  = gibs::sample_irradiance(data, world_pos, normal);
		irradiance_sum		  += float4(fallback.xyz * fallback.w, fallback.w);
	}

	res.max_contribution_tile_surfel_id = max_contribution_surfel_id;
	res.oldest_tile_surfel_id			= surfel_id_oldest;
	res.tile_irradiance					= irradiance_sum.w > 0.f ? irradiance_sum.xyz / irradiance_sum.w : gi_resolve_buffer[px];
	res.tile_surfel_coverage			= coverage;
	res.tile_surfel_count				= tile_entry.surfel_count;
}

void
get_cell_surfel(const gibs_data data, const gibs_lut_data lut_data, float3 world_pos, float3 normal, inout debug_query_data res)
{
	const uint32				 cell_id	  = gibs::cell::calc_id(data, lut_data, world_pos);
	const gibs_cell_surfel_entry surfel_entry = gibs::cell::surfel_entry_arr(data)[cell_id];

	structured_buffer<gibs_cell_surfel> surfel_buffer = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];

	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;
	uint32 surfel_id_oldest			  = invalid_id_uint32;
	uint32 surfel_age_max			  = 0u;

	float near_coverage = 0.f;

	for (uint32 i = 0; i < surfel_entry.surfel_count; ++i)
	{
		const uint32 surfel_id = gibs::cell::cell_to_surfel_id_arr(data)[surfel_entry.offset + i];

		const gibs_cell_surfel surfel		 = surfel_buffer[surfel_id];
		const float3		   surfel_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);
		const uint32		   surfel_age	 = surfel.recycle_data.frame_since_born();

		const float contribution = gibs::calc_near_contribution(length(surfel.position - world_pos), surfel.radius, surfel_normal, normal);

		const float surfel_visibility = gibs::calc_surfel_visibility<false, gibs_cell_surfel>(data, surfel_id, surfel, world_pos);

		const float contribution_vis = contribution * surfel_visibility;

		near_coverage += contribution_vis;

		if (contribution_vis > 0.f)
		{
			if (max_contribution < contribution_vis)
			{
				max_contribution		   = contribution_vis;
				max_contribution_surfel_id = surfel_id;
			}

			if (surfel_age >= surfel_age_max)
			{
				if (surfel_age == surfel_age_max and surfel_id_oldest < surfel_id)
				{
				}
				else
				{
					surfel_id_oldest = surfel_id;
				}

				surfel_age_max = surfel_age;
			}
		}
	}

	res.max_contribution_cell_surfel_id = max_contribution_surfel_id;
	res.oldest_cell_surfel_id			= surfel_id_oldest;
	res.cell_surfel_count				= surfel_entry.surfel_count;
	res.cell_surfel_coverage			= near_coverage;
};

void
get_surfel_probe(const gibs_data data, const gibs_lut_data lut_data, float3 world_pos, float3 normal, inout debug_query_data res)
{
	float3 irradiance = zero<float3>();

	const uint32				cell_id		= gibs::cell::calc_id(data, lut_data, world_pos);
	const gibs_cell_probe_entry probe_entry = gibs::cell::probe_entry_arr(data)[cell_id];

	byte_array<uint32>					 probe_id_arr		  = gibs::cell::cell_to_probe_id_arr(data);
	structured_buffer<gibs_surfel_probe> probe_buffer		  = global_resource_buffer[data.h_surfel_probe_buffer_srv_id];
	structured_buffer<gibs_recycle_data> probe_recycle_buffer = global_resource_buffer[data.h_surfel_probe_recycle_buffer_srv_id];

	float4 irradiance_sum = zero<float4>();
	float4 radiance_sum	  = zero<float4>();

	float  max_contribution_near		  = 0.f;
	uint32 max_contribution_near_probe_id = invalid_id_uint32;
	float  max_contribution_far			  = 0.f;
	uint32 max_contribution_far_probe_id  = invalid_id_uint32;
	uint32 probe_age_max				  = 0u;
	float  contribution_oldest			  = 0.f;
	uint32 probe_id_oldest				  = invalid_id_uint32;

	float surfel_near_coverage_sum = 0.f;
	float surfel_far_coverage_sum  = 0.f;

	float max_contribution_probe_visibility = 0.f;
	float oldest_probe_visibility			= 0.f;

	for (uint32 i = 0; i < probe_entry.probe_count; ++i)
	{
		const uint32 probe_id = probe_id_arr[probe_entry.offset + i];

		const gibs_surfel_probe probe		 = probe_buffer[probe_id];
		const float3			probe_normal = decode_oct_snorm16(probe.normal_oct_snorm16);
		const float				probe_radius = gibs::calc_cell_size(data, lut_data, probe.position);

		const float3 rel	 = probe.position - world_pos;
		const float3 dir	 = normalize(rel);
		const float	 dist_sq = dot(rel, rel);
		const float	 dist	 = sqrt(dist_sq);

		const float sh_depth		= max(0.f, sh1_eval_scalar(probe.depth_sh, -dir));
		const float sh_coverage_far = max(0.f, sh1_eval_scalar(probe.coverage_far_sh, -dir));

		const float visibility = probe_radius * sh_depth > epsilon_1e4
								   ? 1.f - smoothstep(probe_radius * sh_depth, probe_radius * sh_depth * GIBS_PROBE_VIS_FADE_RATIO, dist)
								   : 1.f;

		const float near_contribution = gibs::calc_near_contribution(dist, probe.surfel_radius, probe_normal, normal)
									  * visibility;

		if (near_contribution > 0.f)
		{
			irradiance_sum += float4(decode_r11g11b10(probe.irradiance_r11g11b10), 1.f) * near_contribution;

			surfel_near_coverage_sum += probe.coverage_near * near_contribution;
		}

		const float far_contribution = gibs::calc_far_contribution(rel, dir, probe_normal, probe.surfel_radius, probe_radius, normal)
									 * visibility;

		if (far_contribution > 0.f)
		{
			radiance_sum += float4(decode_r11g11b10(probe.radiance_r11g11b10), 1.f) * far_contribution;

			surfel_far_coverage_sum += sh_coverage_far * far_contribution;
		}


		if (max_contribution_near < near_contribution)
		{
			max_contribution_near_probe_id = probe_id;
			max_contribution_near		   = near_contribution;

			max_contribution_probe_visibility = visibility;
		}

		if (max_contribution_far < far_contribution)
		{
			max_contribution_far_probe_id = probe_id;
			max_contribution_far		  = far_contribution;
		}

		if (near_contribution > 0.f)
		{
			const gibs_recycle_data probe_recycle_data = probe_recycle_buffer[probe_id];
			const uint32			frame_since_born   = probe_recycle_data.frame_since_born();

			if (probe_age_max < frame_since_born)
			{
				probe_id_oldest		= probe_id;
				probe_age_max		= frame_since_born;
				contribution_oldest = near_contribution;

				oldest_probe_visibility = visibility;
			}
		}
	}

	const float3 irradiance_near = irradiance_sum.w > 0.f ? irradiance_sum.xyz / irradiance_sum.w : zero<float3>();
	const float3 irradiance_far	 = radiance_sum.w > 0.f ? pi * radiance_sum.xyz / radiance_sum.w : zero<float3>();

	const float near_conf = irradiance_sum.w / 1.f * GIBS_NEAR_CONTRIBUTION_TRUST_BIAS;
	const float far_conf  = radiance_sum.w / 1.f;

	irradiance = lerp(irradiance_near, irradiance_far, far_conf / (near_conf + far_conf + epsilon_1e4));

	const float probe_near_coverage	 = irradiance_sum.w;
	const float probe_far_coverage	 = radiance_sum.w;
	const float surfel_near_coverage = irradiance_sum.w > 0.f ? surfel_near_coverage_sum / irradiance_sum.w : 0.f;
	const float surfel_far_coverage	 = radiance_sum.w > 0.f ? surfel_far_coverage_sum / radiance_sum.w : 0.f;

	res.max_contribution_probe_id	 = max_contribution_near_probe_id;
	res.oldest_probe_id				 = probe_id_oldest;
	res.cell_irradiance_near		 = irradiance_near;
	res.cell_irradiance_far			 = irradiance_far;
	res.cell_irradiance				 = irradiance;
	res.probe_count					 = probe_entry.probe_count;
	res.probe_near_coverage			 = probe_near_coverage;
	res.probe_far_coverage			 = probe_far_coverage;
	res.cell_surfel_near_sh_coverage = surfel_near_coverage;
	res.cell_surfel_far_sh_coverage	 = surfel_far_coverage;

	res.max_contribution_probe_visibility = max_contribution_probe_visibility;
	res.oldest_probe_visibility			  = oldest_probe_visibility;
};

[numthreads(8, 8, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	if (dispatch_thread_id.x >= (uint32)backbuffer_size.x || dispatch_thread_id.y >= (uint32)backbuffer_size.y)
	{
		return;
	}

	const int32_2 px = dispatch_thread_id.xy;

	const texture_2d<float>	   opaque_depth_tex		 = global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<float>	   transparent_depth_tex = global_resource_buffer[transparent_depth_buffer_srv_id];
	const texture_2d<uint32_2> gbuffer				 = global_resource_buffer[opaque_gbuffer_srv_id];

	const float opaque_z_depth		= opaque_depth_tex[px];
	const float transparent_z_depth = transparent_depth_tex[px];

	if (opaque_z_depth == 0.f and transparent_z_depth == 0.f) { return; }

	float3 normal;
	float3 world_pos;

	debug_query_data debug_data = debug_query_data::init();

	const gibs_data		data	 = gibs::load_data();
	const gibs_lut_data lut_data = gibs::load_lut_data();

	if (transparent_z_depth != 0.f)
	{
		const float3 world_far = ndc_to_world(view_proj_inv, screen_px_to_ndc(px, opaque_z_depth, inv_backbuffer_size));

		const float3 rel	 = world_far - camera_pos;
		const float	 t_max	 = length(rel);
		const float3 ray_dir = rel / t_max;

		ray_desc desc;
		desc.Origin	   = camera_pos;
		desc.Direction = ray_dir;
		desc.TMin	   = 0.f;
		desc.TMax	   = t_max;

		rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

		ray_query<RAY_FLAG_CULL_OPAQUE> query;
		rt_trace_ray_inline(query, tlas, RAY_FLAG_CULL_OPAQUE, RT_MASK_TRANSPARENT, desc);

		while (rt_proceed(query))
		{
			rt_commit_non_opaque_triangle_hit(query);
		}

		if (rt_committed_status(query) != COMMITTED_NOTHING)
		{
			const rt_instance_render_data render_data = load_rt_instance_render_data(rt_committed_instance_id(query));
			const material				  mat		  = load_material(render_data.material_id);
			const object_data			  obj_data	  = load_object_data(render_data.object_id);
			const mesh_header			  msh_header  = read_mesh_header<rt_instance_render_data>(render_data);
			const uint32_3				  prim_index  = load_rt_triangle_index(render_data, rt_committed_primitive_index(query));

			const vertex_fat v0 = decode_vertex(msh_header, prim_index.x);
			const vertex_fat v1 = decode_vertex(msh_header, prim_index.y);
			const vertex_fat v2 = decode_vertex(msh_header, prim_index.z);

			const float2 barycentrics = rt_committed_triangle_barycentrics(query);

			const float3 bary_weights = float3(1.f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

			const vertex_fat v = transform_vertex_to_world(interpolate_vertex_fat(v0, v1, v2, bary_weights), obj_data);

			const float3 local_face_normal = normalize(cross(v1.pos.xyz - v0.pos.xyz, v2.pos.xyz - v0.pos.xyz));

			const float3 world_face_normal = normalize(rotate(local_face_normal / cast<float3>(obj_data.scale), decode_quaternion(obj_data.quaternion)));

			world_pos = v.world_pos;
			normal	  = rt_committed_triangle_front_face(query) ? world_face_normal : -world_face_normal;
		}
		else
		{
			return;
		}

		get_surfel_probe(data, lut_data, world_pos, normal, debug_data);
		get_cell_surfel(data, lut_data, world_pos, normal, debug_data);
	}
	else if (opaque_z_depth != 0.f)
	{
		world_pos = ndc_to_world(view_proj_inv, screen_px_to_ndc(px, opaque_z_depth, inv_backbuffer_size));
		normal	  = decode_oct_snorm16(gbuffer[px].y);

		get_tile_surfel(data, lut_data, px, world_pos, normal, debug_data);
		get_surfel_probe(data, lut_data, world_pos, normal, debug_data);
		get_cell_surfel(data, lut_data, world_pos, normal, debug_data);
	}

	structured_buffer<gibs_tile_surfel>	 tile_surfel_buffer = global_resource_buffer[data.h_tile_surfel_buffer_srv_id];
	structured_buffer<gibs_cell_surfel>	 cell_surfel_buffer = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];
	structured_buffer<gibs_surfel_probe> probe_buffer		= global_resource_buffer[data.h_surfel_probe_buffer_srv_id];

	gibs_tile_surfel  tile_surfel_oldest;
	gibs_cell_surfel  cell_surfel_oldest;
	gibs_surfel_probe probe_oldest;

	if (debug_data.oldest_tile_surfel_id != invalid_id_uint32)
	{
		tile_surfel_oldest = tile_surfel_buffer[debug_data.oldest_tile_surfel_id];
	}
	if (debug_data.oldest_cell_surfel_id != invalid_id_uint32)
	{
		cell_surfel_oldest = cell_surfel_buffer[debug_data.oldest_cell_surfel_id];
	}
	if (debug_data.oldest_probe_id != invalid_id_uint32)
	{
		probe_oldest = probe_buffer[debug_data.oldest_probe_id];
	}

	float4 col = zero<float4>();

	attr_branch()

	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RADIANCE)
	{
		if (debug_data.oldest_tile_surfel_id != invalid_id_uint32)
		{
			col += float4(decode_r11g11b10(tile_surfel_oldest.irradiance_r11g11b10), 1.f);
		}
		if (debug_data.oldest_cell_surfel_id != invalid_id_uint32)
		{
			col += float4(decode_r11g11b10(cell_surfel_oldest.irradiance_r11g11b10), 1.f);
		}
		if (debug_data.oldest_probe_id != invalid_id_uint32)
		{
			col += float4(decode_r11g11b10(probe_oldest.irradiance_r11g11b10), 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_IRRADIANCE)
	{
		// col = debug_data.cell_irradiance;
		col = float4(debug_data.tile_irradiance, 1.f);
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_VISIBILITY)
	{
		if (debug_data.oldest_tile_surfel_id != invalid_id_uint32)
		{
			const float visibility = gibs::calc_surfel_visibility<true, gibs_tile_surfel>(data, debug_data.oldest_tile_surfel_id, tile_surfel_buffer[debug_data.oldest_tile_surfel_id], world_pos);

			col += float4(random_color(debug_data.oldest_tile_surfel_id).rgb * visibility, 1.f);
		}
		if (debug_data.oldest_cell_surfel_id != invalid_id_uint32)
		{
			const float visibility = gibs::calc_surfel_visibility<false, gibs_cell_surfel>(data, debug_data.oldest_cell_surfel_id, cell_surfel_buffer[debug_data.oldest_cell_surfel_id], world_pos);

			col += float4(random_color(debug_data.oldest_cell_surfel_id).rgb * visibility, 1.f);
		}
		if (debug_data.oldest_probe_id != invalid_id_uint32)
		{
			col += float4(random_color(debug_data.oldest_probe_id).rgb * debug_data.oldest_probe_visibility, 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_INSTABILITY)
	{
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RAY_COUNT)
	{
		if (debug_data.tile_surfel_count <= 27)
		{
			col = float4(debug_data.tile_surfel_count / float(27), 0, 0, 1.f);
		}
		else if (debug_data.tile_surfel_count <= 128)
		{
			col = float4(0, debug_data.tile_surfel_count / float(128), 0, 1.f);
		}
		else
		{
			col = float4(0, 0, debug_data.tile_surfel_count / float(256), 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_MSME)
	{
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_ID_HASH)
	{
		if (debug_data.oldest_tile_surfel_id != invalid_id_uint32)
		{
			// col += float4(random_color(debug_data.oldest_tile_surfel_id).rgb, 1.f);
		}
		if (debug_data.oldest_cell_surfel_id != invalid_id_uint32)
		{
			col += float4(random_color(debug_data.oldest_cell_surfel_id).rgb, 1.f);
		}
		if (debug_data.oldest_probe_id != invalid_id_uint32)
		{
			// col += float4(random_color(debug_data.oldest_probe_id).rgb, 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_NORMAL)
	{
		if (debug_data.oldest_tile_surfel_id != invalid_id_uint32)
		{
			col = float4(decode_oct_snorm16(tile_surfel_buffer[debug_data.oldest_tile_surfel_id].normal_oct_snorm16), 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_COVERAGE)
	{
		const float coverage = debug_data.tile_surfel_coverage;

		float ratio = max(0.f, coverage - GIBS_TILE_SURFEL_SPAWN_COVERAGE) / float(GIBS_TILE_SURFEL_KILL_COVERAGE - GIBS_TILE_SURFEL_SPAWN_COVERAGE);

		if (coverage >= GIBS_TILE_SURFEL_KILL_COVERAGE)
		{
			col = color_red;
		}
		else if (coverage <= GIBS_TILE_SURFEL_SPAWN_COVERAGE)
		{
			col = color_blue;
		}
		else
		{
			col = float4(0, ratio, 0, 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_AGE)
	{
		if (debug_data.oldest_tile_surfel_id != invalid_id_uint32)
		{
			const uint32 age = tile_surfel_buffer[debug_data.oldest_tile_surfel_id].recycle_data.frame_since_born();
			col				 = float4(age / float(0xff), age / float(0xf), age, 1.f);
		}
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_CELL)
	{
		col = float4(random_color(gibs::cell::calc_id(data, lut_data, world_pos)), 1.f);
	}
	else if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_VARIANCE)
	{
	}

	rw_texture_2d<float4> res_tex = global_resource_buffer[blend_buffer_uav_id];
	res_tex[px]					  = col;
};
