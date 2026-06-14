#include "forward_plus_common.asli"

void
gibs_trace_ray(float3 pos, float3 dir, out float res_distance, out uint32 res_radiance_r11g11b10)
{
	float  distance;
	float3 color;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.05f;
	desc.TMax	   = float_max;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;
		// ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;


		color = rt_calc_opaque_color(desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			distance = float_max;
		}
		else if (rt_committed_triangle_front_face(query) is_false)
		{
			// distance = -rt_committed_ray_t(query);
			distance = rt_committed_ray_t(query);
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

		const float4 transparent_color = rt_calc_transparent_color(desc, query);

		color = color * (1.f - transparent_color.a) + transparent_color.rgb;
	}

	res_distance = distance;

	assert(all(color >= 0.f), g::fmt_forward_plus_gibs_ray_trace_cs, line, color);
	assert(is_nan(color) is_false, g::fmt_forward_plus_gibs_ray_trace_cs, line, color);

	res_radiance_r11g11b10 = encode_r11g11b10(color);
}

// helper
template <typename t_tex>
struct luminance_cdf_arr
{
	t_tex tex;

	uint32_2 offset;

	static luminance_cdf_arr
	init(t_tex tex, uint32_2 offset)
	{
		luminance_cdf_arr res;
		res.tex	   = tex;
		res.offset = offset;
		return res;
	}

	float
	operator[](uint32 idx_flat)
	{
		uint32_2 px = offset + uint32_2(idx_flat % GIBS_ATLAS_TILE_SIZE, idx_flat / GIBS_ATLAS_TILE_SIZE);
		return tex[px].y;
	}

	void
	store(uint32 idx_flat, float y)
	{
		uint32_2 px		  = offset + uint32_2(idx_flat % GIBS_ATLAS_TILE_SIZE, idx_flat / GIBS_ATLAS_TILE_SIZE);
		float2	 original = tex[px];
		tex[px]			  = float2(original.x, y);
	}
};

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 group_id sv_group_id,
		uint32 ray_id	sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	if (ray_id >= gibs_get_ray_count_total(data)) { return; }

	const gibs_ray_entry ray_entry = gibs_load_ray_entry_rw(data, ray_id);

	const surfel surfel = gibs_load_surfel_arr(data)[ray_entry.surfel_id];

	const float4 rand_4d = random_pcg4d(uint32_4(ray_id, ray_entry.surfel_id, frame_index, ray_id + ray_entry.surfel_id + frame_index));

	float3					 dir_local;
	float					 pdf;
	const texture_2d<float2> irradiance_atlas = global_resource_buffer[data.h_irradiance_atlas_srv_id];

	const uint32_2 atlas_offset = gibs_calc_atlas_offset(data, ray_entry.surfel_id);
	assert(ray_entry.surfel_id < data.max_surfel_count, g::fmt_forward_plus_gibs_ray_trace_cs, line);

	const float luminance_sum = irradiance_atlas[atlas_offset + uint32_2(GIBS_ATLAS_TILE_SIZE - 1, GIBS_ATLAS_TILE_SIZE - 1)].y;
	bool		ray_guided	  = false;

	const surfel_recycle_data recycle = gibs_load_surfel_recycle_data_rw_arr(data)[ray_entry.surfel_id];

	float pdf_guide_test;
	float pdf_cos_test;
	if (luminance_sum > GIBS_MIN_LUMINANCE_FOR_RAY_GUIDANCE)
	{
		float		pdf_guide	   = 0.f;
		float		pdf_cos		   = 0.f;
		const float ray_guide_prob = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born)) * 0.6f;
		// clang-format off
		luminance_cdf_arr<texture_2d<float2> > luminance_cdf = luminance_cdf_arr<texture_2d<float2> >::init(global_resource_buffer[data.h_irradiance_atlas_srv_id], atlas_offset);
		// clang-format on
		if (rand_4d.w < ray_guide_prob)
		{
			ray_guided = true;
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


			pdf_guide_test = pdf_guide;
			pdf_cos_test   = pdf_cos;
			// if (ray_id == 0)
			//{
			//	for (uint32 i = 0; i < GIBS_ATLAS_TILE_SIZE; ++i)
			//	{
			//		for (uint32 j = 0; j < GIBS_ATLAS_TILE_SIZE; ++j)
			//		{
			//			const uint32 idx = i * GIBS_ATLAS_TILE_SIZE + j;
			//			debug_log(g::fmt_forward_plus_gibs_ray_trace_cs, ray_id,
			//					  uint32_2(i, j), luminance_cdf[idx]);
			//		}
			//	}
			// }
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


			// if (pdf_guide < 0.f and wave_is_first_lane())
			//{
			//	texture_2d<float2> tex = global_resource_buffer[data.h_irradiance_atlas_srv_id];
			//	for (uint32 v = 0; v < GIBS_ATLAS_TILE_SIZE; ++v)
			//	{
			//		for (uint32 u = 0; u < GIBS_ATLAS_TILE_SIZE; ++u)
			//		{
			//			uint32_2 px = atlas_offset + uint32_2(u, v);

			//			const uint32 idx = v * GIBS_ATLAS_TILE_SIZE + u;
			//			debug_log(g::fmt_forward_plus_gibs_ray_trace_cs, ray_id,
			//					  v, u, tex[px]);
			//		}
			//	}
			//}

			// assert(pdf_guide >= 0.f, g::fmt_forward_plus_gibs_ray_trace_cs, line, uv, texel, idx, p_texel, pdf_uv, pdf_w);
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

	assert(pdf > 0.f, g::fmt_forward_plus_gibs_ray_trace_cs, line, pdf);

	const float3 dir_world = mul(dir_local, gen_world_normal_transform_t(decode_oct_snorm16(surfel.normal_oct_snorm16)));

	gibs_ray_result res;
	gibs_trace_ray(surfel.position, dir_world, /*out*/ res.distance, /*out*/ res.radiance_r11g11b10);
	res.dir_oct_snorm8 = uint32(encode_world_hemi_oct_snorm8(dir_local)) | (uint32(encode_oct_snorm8(dir_world)) << 16u);
	res.pdf			   = pdf;

	gibs_load_ray_result_rw_arr(data).store(ray_id, res);


	// if (surfel.alive_idx == 400 and ray_guided)
	//{
	//	debug_log(g::fmt_forward_plus_gibs_ray_trace_cs, ray_entry.local_ray_id, dir_world, decode_r11g11b10(res.radiance_r11g11b10), res.pdf, length(camera_pos - surfel.position), pdf_guide_test, pdf_cos_test);
	// }
}