#include "hrp_common.asli"

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
			const float pdf_w  = pdf_uv / math::jacobian::calc_hemi_oct(uv * 2.f - 1.f);
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
			const float pdf_w  = pdf_uv / math::jacobian::calc_hemi_oct(uv * 2.f - 1.f);
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

	gibs_ray_hit_result res = rt::trace_ray<gibs_ray_hit_result>(ray_id, surface_offset(world_pos, surfel_normal), dir_world);
	res.dir_oct_snorm8		= uint32(encode_world_hemi_oct_snorm8(dir_local)) | (uint32(encode_oct_snorm8(dir_world)) << 16u);
	res.pdf					= pdf;

	rw_structured_buffer<gibs_ray_hit_result> ray_hit_buffer = global_resource_buffer[data.h_ray_hit_buffer_uav_id];

	ray_hit_buffer[ray_id] = res;
}