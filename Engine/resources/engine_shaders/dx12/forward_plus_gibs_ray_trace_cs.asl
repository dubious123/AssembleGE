#include "forward_plus_common.asli"

void
gibs_trace_ray(float3 pos, float3 dir, out float res_distance, out uint32 res_radiance_r11g11b10)
{
	float  distance;
	float3 color;

	ray_desc desc;
	desc.Origin	   = pos;
	desc.Direction = dir;
	desc.TMin	   = 0.001f;
	desc.TMax	   = float_max;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	// check opaque
	{
		ray_query<RAY_FLAG_NONE> query;

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

	res_distance		   = distance;
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

	const gibs_ray_entry ray_entry = gibs_load_ray_entry_rw_arr(data)[ray_id];

	const surfel surfel = gibs_load_surfel_rw_arr(data)[ray_entry.surfel_id];

	const float3 rand_3d = random_pcg3d(uint32_3(ray_id, ray_entry.surfel_id, frame_index));

	float3					 dir_local;
	float					 pdf;
	const texture_2d<float2> irradiance_atlas = global_resource_buffer[data.h_irradiance_atlas_srv_id];

	const uint32_2 atlas_offset = gibs_calc_atlas_offset(data, ray_entry.surfel_id);

	const float luminance_sum = irradiance_atlas[atlas_offset + uint32_2(GIBS_ATLAS_TILE_SIZE - 1, GIBS_ATLAS_TILE_SIZE - 1)].y;
	if (luminance_sum > GIBS_MIN_LUMINANCE_FOR_RAY_GUIDANCE)
	{
		// clang-format off
		luminance_cdf_arr< texture_2d<float2> > luminance_cdf = luminance_cdf_arr< texture_2d<float2> >::init(global_resource_buffer[data.h_irradiance_atlas_srv_id], atlas_offset);
		// clang-format on
		// idx is always less than GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE
		const uint32 idx = upper_boundary(luminance_cdf, 0, GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1, rand_3d.x);

		const float2 jitter = rand_3d.yz;
		const float2 uv		= saturate((int32_2(idx % GIBS_ATLAS_TILE_SIZE, idx / GIBS_ATLAS_TILE_SIZE) + jitter) / float(GIBS_ATLAS_TILE_SIZE));
		dir_local			= decode_octahedral(uv * 2.f - 1.f);

		pdf = idx == 0
				? luminance_cdf[idx]
			: idx == GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE - 1
				? 1.f - luminance_cdf[idx - 1]
				: luminance_cdf[idx] - luminance_cdf[idx - 1];
	}
	else
	{
		// fall back to cos sampling
		dir_local = sample_hemisphere_cosine_local(rand_3d.xy);
		pdf		  = dir_local.z * pi_inv;	 // dir_local.z == cos_theta
	}

	const float3 dir_world = mul(dir_local, gen_normal_transform_t(decode_oct_snorm16(surfel.normal_oct_snorm16)));

	gibs_ray_result res;
	gibs_trace_ray(surfel.position, dir_world, /*out*/ res.distance, /*out*/ res.radiance_r11g11b10);
	res.dir_oct_snorm8 = encode_oct_snorm8(dir_world) | (encode_oct_snorm8(dir_local) << 16u);
	res.pdf			   = pdf;


	gibs_load_ray_result_rw_arr(data).store(ray_id, res);
}