#include "hrp_common.asli"

template <uint32 rt_flags, bool is_opaque>
void
fill_hit_committed(inout gibs_ray_hit_result res, ray_query<rt_flags> query)
{
	res.distance	 = rt_committed_ray_t(query);
	res.object_id	 = load_rt_instance_render_data(rt_committed_instance_id(query)).object_id;
	res.primitive_id = rt_committed_primitive_index(query);

	const float2 bary = rt_committed_triangle_barycentrics(query);

	res.barycentric_unorm16 = uint32(float_to_unorm16(bary.x))
							| (uint32(float_to_unorm16(bary.y)) << 16u);

	if (rt_committed_triangle_front_face(query) is_false)
	{
		res.distance = -res.distance;

		if (is_opaque)
		{
			res.object_id = invalid_id_uint32;
		}
	}
}

gibs_ray_hit_result
gibs_trace_ray(const uint32 seed, float3 pos, float3 dir)
{
	gibs_ray_hit_result res = zero<gibs_ray_hit_result>();
	res.object_id			= invalid_id_uint32;
	res.distance			= float_max;

	ray_desc desc;
	// desc.Origin	   = pos;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.0f;
	desc.TMax	   = float_max;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];
	{
		ray_query<RAY_FLAG_CULL_NON_OPAQUE> query;
		rt_trace_ray_inline(query, tlas, RAY_FLAG_CULL_NON_OPAQUE, RT_MASK_OPAQUE, desc);
		rt_proceed(query);

		if (rt_committed_status(query) == COMMITTED_NOTHING)
		{
			res.distance = float_max;
		}
		else
		{
			fill_hit_committed<RAY_FLAG_CULL_NON_OPAQUE, true>(res, query);
		}
	}

	if (res.distance > 0.f)	   // not inside wall
	{
		desc.TMax = res.distance;

		ray_query<RAY_FLAG_CULL_OPAQUE> query;
		rt_trace_ray_inline(query, tlas, RAY_FLAG_CULL_OPAQUE, RT_MASK_TRANSPARENT, desc);

		while (rt_proceed(query))
		{
			const uint32				  rt_instance_render_data_id = rt_candidate_instance_id(query);
			const uint32				  primitive_id				 = rt_candidate_primitive_index(query);
			const float2				  barycentrics				 = rt_candidate_triangle_barycentrics(query);
			const rt_instance_render_data render_data				 = load_rt_instance_render_data(rt_instance_render_data_id);
			const material				  mat						 = load_material(render_data.material_id);

			float4 base_color = mat.base_color_factor;
			if (mat.base_color_texture_id != invalid_id_uint32)
			{
				const object_data obj		 = load_object_data(render_data.object_id);
				const mesh_header msh_header = read_mesh_header<rt_instance_render_data>(render_data);
				const uint32_3	  prim_index = load_rt_triangle_index(render_data, primitive_id);

				const vertex_fat v0 = decode_vertex(msh_header, prim_index.x);
				const vertex_fat v1 = decode_vertex(msh_header, prim_index.y);
				const vertex_fat v2 = decode_vertex(msh_header, prim_index.z);

				const float3 bary_weights = float3(1.f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

				const vertex_fat v = transform_vertex_to_world(interpolate_vertex_fat(v0, v1, v2, bary_weights), obj);

				texture_2d<float4> base_color_tex  = global_resource_buffer[non_uniform_resource_idx(mat.base_color_texture_id)];
				base_color						  *= sample(base_color_tex, get_linear_wrap_sampler(), v.uv_set[0]);
			}

			const float rng = random_pcg4d(uint32_4(seed, rt_instance_render_data_id, primitive_id, frame_index)).x;

			if (rng < base_color.a)
			{
				rt_commit_non_opaque_triangle_hit(query);
			}
		}

		if (rt_committed_status(query) != COMMITTED_NOTHING)
		{
			fill_hit_committed<RAY_FLAG_CULL_OPAQUE, false>(res, query);
		}
	}

	return res;
}

wave_size(AGE_WAVE_SIZE)
[numthreads(64, 1, 1)] void
main_cs(uint32 group_id sv_group_id,
		uint32 ray_id	sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	const uint32_2 ray_count_total			   = gibs::ray::count_total(data);
	const uint32   tile_surfel_ray_count_total = ray_count_total.x;
	const uint32   cell_surfel_ray_count_total = ray_count_total.y;

	const bool is_tile = ray_id < tile_surfel_ray_count_total;

	if (ray_id >= ray_count_total.x + ray_count_total.y) { return; }

	const gibs_ray_entry ray_entry = gibs::ray::load_ray_entry(data, ray_id);

	const uint32 surfel_id = ray_entry.surfel_id;

	float3 world_pos;
	float3 surfel_normal;
	float3 dir_local;
	float  pdf;


	float ray_guide_prob;

	if (is_tile)
	{
		structured_buffer<gibs_tile_surfel> surfel_arr = global_resource_buffer[data.h_tile_surfel_buffer_srv_id];

		const gibs_tile_surfel surfel = surfel_arr[surfel_id];

		world_pos	  = surfel.position;
		surfel_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);

		ray_guide_prob = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.recycle_data.frame_since_born())) * 0.95f;
	}
	else
	{
		structured_buffer<gibs_cell_surfel> surfel_arr = global_resource_buffer[data.h_cell_surfel_buffer_srv_id];

		const gibs_cell_surfel surfel = surfel_arr[surfel_id];

		world_pos	  = surfel.position;
		surfel_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);

		ray_guide_prob = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.recycle_data.frame_since_born())) * 0.95f;
	}

	byte_array<half> luminance_cdf = gibs::load_lum_cdf_arr(data, surfel_id, is_tile);

	const float luminance_sum = luminance_cdf[GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1];

	const float4 rand_4d = random_pcg4d(uint32_4(ray_id, surfel_id, frame_index, ray_id + surfel_id + frame_index));

	if (luminance_sum > GIBS_MIN_LUMINANCE_FOR_RAY_GUIDANCE)
	{
		float pdf_guide = 0.f;
		float pdf_cos	= 0.f;

		if (rand_4d.w < ray_guide_prob)
		{
			// idx is always less than GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE
			const uint32 idx = upper_boundary(luminance_cdf, 0, GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1, rand_4d.x);


			const float2 jitter = rand_4d.yz;
			const float2 uv		= saturate((int32_2(idx % GIBS_ATLAS_TILE_SIZE, idx / GIBS_ATLAS_TILE_SIZE) + jitter) / float(GIBS_ATLAS_TILE_SIZE));
			dir_local			= decode_world_hemi_octahedral(uv * 2.f - 1.f);


			const float p_texel = idx == 0
									? luminance_cdf[idx]
								: idx == GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1
									? 1.f - luminance_cdf[idx - 1]
									: max(0.f, luminance_cdf[idx] - luminance_cdf[idx - 1]);
			// pdf_uv = p_texel / (4.f / float(GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE));
			const float pdf_uv = max(p_texel, epsilon_1e4) * float(GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE) * 0.25f;
			const float pdf_w  = pdf_uv / calc_hemi_oct_jacobian(uv * 2.f - 1.f);
			pdf_guide		   = pdf_w;

			pdf_cos = max(epsilon_1e4, dir_local.y) * pi_inv;
		}
		else
		{
			// fall back to cos sampling

			dir_local		 = sample_tbn_hemisphere_cosine_local(rand_4d.xy);
			const float temp = dir_local.y;
			dir_local.y		 = dir_local.z;
			dir_local.z		 = temp;

			pdf_cos = max(epsilon_1e4, dir_local.y) * pi_inv;	 // dir_local.y == cos_theta


			const float2  uv	= saturate(encode_world_hemi_octahedral(dir_local) * 0.5f + 0.5f);
			const int32_2 texel = min(int32_2(uv * float(GIBS_ATLAS_TILE_SIZE)), int32_2(GIBS_ATLAS_TILE_SIZE - 1, GIBS_ATLAS_TILE_SIZE - 1));
			const uint32  idx	= texel.y * GIBS_ATLAS_TILE_SIZE + texel.x;

			const float p_texel = idx == 0
									? luminance_cdf[idx]
								: idx == GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1
									? 1.f - luminance_cdf[idx - 1]
									: max(0.f, luminance_cdf[idx] - luminance_cdf[idx - 1]);

			const float pdf_uv = p_texel * float(GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE) * 0.25f;
			const float pdf_w  = pdf_uv / calc_hemi_oct_jacobian(uv * 2.f - 1.f);
			pdf_guide		   = pdf_w;
		}

		pdf = ray_guide_prob * pdf_guide + (1.f - ray_guide_prob) * pdf_cos;
	}
	else
	{
		// fall back to cos sampling
		dir_local		 = sample_tbn_hemisphere_cosine_local(rand_4d.xy);
		const float temp = dir_local.y;
		dir_local.y		 = dir_local.z;
		dir_local.z		 = temp;

		pdf = max(epsilon_1e4, dir_local.y) * pi_inv;	 // dir_local.y == cos_theta
	}

	assert(pdf > 0.f, line, pdf);

	const float3 dir_world = mul(dir_local, gen_world_normal_transform_t(surfel_normal));

	gibs_ray_hit_result res = gibs_trace_ray(ray_id, surface_offset(world_pos, surfel_normal), dir_world);
	// gibs_trace_ray(rt_arg::init_gibs(surfel.surfel_seen()), surfel.position + 0.1f * surfel.radius * normal, normal, dir_world, /*out*/ res.distance, /*out*/ res.radiance_r11g11b10);
	res.dir_oct_snorm8 = uint32(encode_world_hemi_oct_snorm8(dir_local)) | (uint32(encode_oct_snorm8(dir_world)) << 16u);
	res.pdf			   = pdf;

	rw_structured_buffer<gibs_ray_hit_result> ray_hit_buffer = global_resource_buffer[data.h_ray_hit_buffer_uav_id];

	ray_hit_buffer[ray_id] = res;
}