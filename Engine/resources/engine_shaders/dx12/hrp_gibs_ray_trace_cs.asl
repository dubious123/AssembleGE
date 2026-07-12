#include "hrp_common.asli"

// from RT Gems Ch.6
// A Fast and Robust Method for Avoiding Self-Intersection
float3
offset_ray(float3 p, float3 n)
{
	const float origin		= 1.0f / 32.0f;
	const float float_scale = 1.0f / 65536.0f;
	const float int_scale	= 256.0f;

	int32_3 of_i = int32_3(int_scale * n.x, int_scale * n.y, int_scale * n.z);

	float3 p_i = float3(
		as_float(as_int(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		as_float(as_int(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		as_float(as_int(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return float3(
		abs(p.x) < origin ? p.x + float_scale * n.x : p_i.x,
		abs(p.y) < origin ? p.y + float_scale * n.y : p_i.y,
		abs(p.z) < origin ? p.z + float_scale * n.z : p_i.z);
}

void
gibs_trace_ray(const rt_arg arg, float3 pos, float3 normal, float3 dir, out float res_distance, out uint32 res_radiance_r11g11b10)
{
	float  distance;
	float3 color;

	ray_desc desc;
	// desc.Origin	   = pos;
	desc.Origin	   = offset_ray(pos, normal);
	desc.Direction = dir;
	desc.TMin	   = 0.0f;
	desc.TMax	   = float_max;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;
		// ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

		color = rt_calc_opaque_color(arg, desc, query);

		const uint32 status = rt_committed_status(query);

		if (status == COMMITTED_NOTHING)
		{
			distance = float_max;
		}
		else if (rt_committed_triangle_front_face(query) is_false)
		{
			distance = -rt_committed_ray_t(query);
			// distance = rt_committed_ray_t(query);
		}
		else
		{
			distance = rt_committed_ray_t(query);
		}
	}

	assert(all(color >= 0.f), g::fmt_forward_plus_gibs_ray_trace_cs, line, color);
	assert(is_nan(color) is_false, g::fmt_forward_plus_gibs_ray_trace_cs, line, color);

	if (distance > 0.f)	   // not inside wall
	{
		desc.TMax = distance;

		ray_query<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;

		const float4 transparent_color = rt_calc_transparent_color(arg, desc, query);

		color = color * (1.f - transparent_color.a) + transparent_color.rgb;
	}

	res_distance = distance;

	assert(all(color >= 0.f), g::fmt_forward_plus_gibs_ray_trace_cs, line, color);
	assert(is_nan(color) is_false, g::fmt_forward_plus_gibs_ray_trace_cs, line, color);

	res_radiance_r11g11b10 = encode_r11g11b10(color);
}

wave_size(32)
[numthreads(64, 1, 1)] void
main_cs(uint32 group_id sv_group_id,
		uint32 ray_id	sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	if (ray_id >= gibs_load_ray_count_total(data)) { return; }

	const gibs_ray_entry ray_entry = gibs_load_ray_entry_rw(data, ray_id);

	const surfel surfel = gibs_load_surfel_arr(data)[ray_entry.surfel_id];

	const float4 rand_4d = random_pcg4d(uint32_4(ray_id, ray_entry.surfel_id, frame_index, ray_id + ray_entry.surfel_id + frame_index));

	float3 dir_local;
	float  pdf;

	assert(ray_entry.surfel_id < data.max_surfel_count, g::fmt_forward_plus_gibs_ray_trace_cs, line);
	const byte_array<half> luminance_cdf = gibs_load_lum_cdf_arr(data, ray_entry.surfel_id);
	const float			   luminance_sum = luminance_cdf[GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1];

	if (luminance_sum > GIBS_MIN_LUMINANCE_FOR_RAY_GUIDANCE)
	{
		float		pdf_guide	   = 0.f;
		float		pdf_cos		   = 0.f;
		const float ray_guide_prob = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born())) * 0.95f;

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

	assert(pdf > 0.f, g::fmt_forward_plus_gibs_ray_trace_cs, line, pdf);

	const float3 normal	   = decode_oct_snorm16(surfel.normal_oct_snorm16);
	const float3 dir_world = mul(dir_local, gen_world_normal_transform_t(normal));

	gibs_ray_result res;
	gibs_trace_ray(rt_arg::init_gibs(true), surfel.position /*+ 0.1f * surfel.radius * normal*/, normal, dir_world, /*out*/ res.distance, /*out*/ res.radiance_r11g11b10);
	// gibs_trace_ray(rt_arg::init_gibs(surfel.surfel_seen()), surfel.position + 0.1f * surfel.radius * normal, normal, dir_world, /*out*/ res.distance, /*out*/ res.radiance_r11g11b10);
	res.dir_oct_snorm8 = uint32(encode_world_hemi_oct_snorm8(dir_local)) | (uint32(encode_oct_snorm8(dir_world)) << 16u);
	res.pdf			   = pdf;

	gibs_load_ray_result_rw_arr(data).store(ray_id, res);
}