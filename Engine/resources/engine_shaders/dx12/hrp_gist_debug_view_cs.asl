#include "hrp_common.asli"

struct debug_query_data
{
	// cell
	uint32 cell_id;

	uint32 max_contribution_cell_surfel_id;
	uint32 oldest_cell_surfel_id;
	uint32 cell_surfel_count;
	float  cell_surfel_coverage;

	float oldest_cell_surfel_visibility;

	// tile
	uint32 tile_id;

	uint32 max_contribution_tile_surfel_id;
	uint32 oldest_tile_surfel_id;
	float3 tile_irradiance;
	float  tile_surfel_coverage;
	uint32 tile_surfel_count;
	float  oldest_tile_surfel_visibility;

	static debug_query_data
	init()
	{
		debug_query_data res				= zero<debug_query_data>();
		res.max_contribution_cell_surfel_id = invalid_id_uint32;
		res.oldest_cell_surfel_id			= invalid_id_uint32;
		res.max_contribution_tile_surfel_id = invalid_id_uint32;
		res.oldest_tile_surfel_id			= invalid_id_uint32;
		return res;
	}
};

void
get_cell_surfel(const gist_data data, const gist_lut_data lut_data, float3 world_pos, float3 normal, inout debug_query_data res)
{
	const uint32				 cell_id	  = gist::cell::calc_id(data, lut_data, world_pos);
	const gist_cell_surfel_entry surfel_entry = gist::cell::surfel_entry_arr(data)[cell_id];

	structured_buffer<gist_cell_surfel> surfel_buffer = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];

	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;
	uint32 surfel_id_oldest			  = invalid_id_uint32;
	uint32 surfel_age_max			  = 0u;
	float  oldest_surfel_visibility	  = 0.f;

	float near_coverage = 0.f;

	for (uint32 i = 0; i < surfel_entry.surfel_count; ++i)
	{
		const uint32 surfel_id = gist::cell::cell_to_surfel_id_arr(data)[surfel_entry.offset + i];

		const gist_cell_surfel surfel		 = surfel_buffer[surfel_id];
		const float3		   surfel_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);
		const uint32		   surfel_age	 = surfel.recycle_data.frame_since_born();

		const float contribution = gist::calc_near_contribution(length(surfel.position - world_pos), surfel.radius, surfel_normal, normal);

		const float surfel_visibility = gist::calc_surfel_visibility<false>(data, surfel_id, surfel, world_pos);

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
					surfel_id_oldest		 = surfel_id;
					oldest_surfel_visibility = surfel_visibility;
				}

				surfel_age_max = surfel_age;
			}
		}
	}

	res.cell_id							= cell_id;
	res.max_contribution_cell_surfel_id = max_contribution_surfel_id;
	res.oldest_cell_surfel_id			= surfel_id_oldest;
	res.cell_surfel_count				= surfel_entry.surfel_count;
	res.cell_surfel_coverage			= near_coverage;
	res.oldest_cell_surfel_visibility	= oldest_surfel_visibility;
};

[numthreads(8, 8, 1)] void
main_cs(uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	const int32_2 extent = int32_2(backbuffer_size);

	if (any(dispatch_thread_id.xy >= extent))
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

	const gist_data		data	 = gist::load_data();
	const gist_lut_data lut_data = gist::load_lut_data();

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

		get_cell_surfel(data, lut_data, world_pos, normal, debug_data);
	}
	else if (opaque_z_depth != 0.f)
	{
		world_pos = ndc_to_world(view_proj_inv, screen_px_to_ndc(px, opaque_z_depth, inv_backbuffer_size));
		normal	  = decode_oct_snorm16(gbuffer[px].y);

		get_cell_surfel(data, lut_data, world_pos, normal, debug_data);
	}


	float4 col = float4(0, 0, 0, 0.f);

	bool tile_surfel_exists = false;
	bool cell_surfel_exists = false;
};
