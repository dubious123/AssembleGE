#include "hrp_common.asli"
#define max_surfel_per_cell 1000

//
// struct fn_foreach_nbr_cell
//{
//	float3 world_pos;
//	float3 px_normal;
//
//	uint32 surfel_id_arr[max_surfel_per_cell];
//	uint32 size;
//
//	int32_4 cell_idx_arr[27];
//	uint32	cell_idx_size;
//
//
//	float contribution_sum;
//
//	uint32 valid_count;
//
//	static fn_foreach_nbr_cell
//	init(float3 world_pos, float3 px_normal, uint32 valid_count)
//	{
//		fn_foreach_nbr_cell res;
//
//		res.contribution_sum = 0.f;
//		res.size			 = 0u;
//		res.world_pos		 = world_pos;
//		res.px_normal		 = px_normal;
//
//		res.cell_idx_size = 0u;
//
//		res.valid_count = valid_count;
//
//		assert(valid_count <= max_surfel_per_cell, g::fmt_forward_plus_gibs_debug_draw_surfels_ps);
//
//		return res;
//	}
//
//	void
//	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
//	{
//		const uint32					  cell_idx_flat		= gibs_flatten_cell_idx(data, cell_idx);
//		const byte_array<uint32>		  cell_to_surfel_id = gibs_load_cell_to_surfel_id_arr(data);
//		const byte_array<gibs_cell_entry> cell_entry_arr	= gibs_load_cell_entry_arr(data);
//		const rw_array<surfel>			  surfel_arr		= gibs_load_surfel_rw_arr(data);
//		const gibs_cell_entry			  cell_entry		= cell_entry_arr[cell_idx_flat];
//
//
//		for (uint32 i = 0; i < cell_idx_size; ++i)
//		{
//			assert(all(cell_idx == cell_idx_arr[i]) is_false, g::fmt_forward_plus_gibs_debug_draw_surfels_ps);
//			assert(cell_idx_flat != gibs_flatten_cell_idx(data, cell_idx_arr[i]), g::fmt_forward_plus_gibs_debug_draw_surfels_ps);
//		}
//
//		cell_idx_arr[cell_idx_size++] = cell_idx;
//
//		for (uint32 i = 0; i < cell_entry.count; ++i)
//		{
//			const uint32 surfel_id	  = cell_to_surfel_id[cell_entry.offset + i];
//			const surfel surfel		  = surfel_arr[surfel_id];
//			const float	 contribution = gibs_calc_surfel_contribution(data, surfel, world_pos, px_normal);
//
//			assert(contribution >= 0.f and contribution <= 1.f, g::fmt_forward_plus_gibs_debug_draw_surfels_ps);
//
//			if (contribution == 0.f) { continue; }
//
//			bool found = false;
//			for (uint32 j = 0; j < size; ++j)
//			{
//				if (surfel_id_arr[j] == surfel_id)
//				{
//					found = true;
//					break;
//				}
//			}
//
//			if (found is_false)
//			{
//				contribution_sum += contribution;
//
//				surfel_id_arr[size++] = surfel_id;
//
//				assert(size <= max_surfel_per_cell, g::fmt_forward_plus_gibs_debug_draw_surfels_ps);
//			}
//		}
//
//		assert(size <= valid_count, g::fmt_forward_plus_gibs_debug_draw_surfels_ps);
//	}
//};

float4
main_ps(float4 pos sv_position) sv_target_0
{
	const gibs_data					  data				= gibs_load_gibs_data();
	const gibs_lut_data				  lut_data			= gibs_load_gibs_lut_data();
	const texture_2d<float>			  depth_tex			= global_resource_buffer[opaque_depth_buffer_srv_id];
	const texture_2d<uint32_2>		  gbuffer			= global_resource_buffer[opaque_gbuffer_srv_id];
	const byte_array<gibs_tile_entry> tile_entry_arr	= gibs_load_tile_entry_arr(data);
	const byte_array<uint32>		  tile_to_surfel_id = gibs_load_tile_to_surfel_id_arr(data);
	const byte_array<gibs_cell_entry> cell_entry_arr	= gibs_load_cell_entry_arr(data);
	const byte_array<uint32>		  cell_to_surfel_id = gibs_load_cell_to_surfel_id_arr(data);
	const byte_array<surfel>		  surfel_arr		= gibs_load_surfel_arr(data);
	const byte_array<surfel_geometry> geo_arr			= gibs_load_surfel_geometry_arr(data);
	const byte_array<surfel_msme>	  msme_arr			= gibs_load_surfel_msme_arr(data);

	uint32_2 px = uint32_2(pos.xy);

	float2 uv = pos.xy * inv_backbuffer_size;

	const float z_depth = load(depth_tex, px.x, px.y, 0);

	if (z_depth == 0.f)
	{
		return float4(0, 0, 0, 0);
	}
	const uint32 object_id			   = load_opaque_meshlet_render_data(gbuffer[px].x & 0x01ffffff).object_id;
	const uint32 px_normal_oct_snorm16 = gbuffer[px].y;
	const float3 px_normal			   = decode_oct_snorm16(px_normal_oct_snorm16);

	float2 ndc = screen_to_ndc(pos.xy, inv_backbuffer_size);

	const float4 clip_pos  = mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	const float3 world_pos = clip_pos.xyz / clip_pos.w;

	const int32_4 cell_idx		= gibs_calc_cell_idx(data, lut_data, world_pos);
	const uint32  cell_idx_flat = gibs_flatten_cell_idx(data, cell_idx);

	const uint32 tile_idx_flat = gibs_flatten_tile_idx(data, px / GIBS_GI_RESOLVE_TILE_SIZE);

	const gibs_tile_entry tile_entry = tile_entry_arr[tile_idx_flat];
	const gibs_cell_entry cell_entry = cell_entry_arr[cell_idx_flat];

	float  min_contribution			  = float_max;
	uint32 min_contribution_surfel_id = invalid_id_uint32;

	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;

	uint32 surfel_id_smallest = invalid_id_uint32;
	uint32 surfel_id_largest  = invalid_id_uint32;
	float  surfel_radius_max  = 0.f;

	uint32 surfel_id_closest = invalid_id_uint32;
	float  surfel_dist_min	 = float_max;

	uint32 surfel_id_oldest = invalid_id_uint32;
	uint32 surfel_age_max	= 0;

	// radiance for new_born, w == weight_sum
	float4 radiance = (float4)0;
	float  coverage = 0.f;

	int32 valid_count = 0;

	for (uint32 i = 0; i < tile_entry.surfel_count(); ++i)
	{
		const uint32 surfel_id	  = tile_to_surfel_id[tile_entry.offset + i];
		const surfel surfel		  = surfel_arr[surfel_id];
		const float	 contribution = gibs_calc_surfel_contribution<false>(data, surfel, world_pos, px_normal);

		const float3 surfel_radiance = decode_r11g11b10(surfel.radiance_r11g11b10);

		assert(is_nan(surfel_radiance) is_false, g::fmt_gibs_gi_resolve, line);

		// assert(gibs_load_alive_surfel_id_stack_curr(data)[surfel.alive_idx] == surfel_id, g::fmt_gibs_gi_resolve, line);

		// const float fallback_contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);

		if (surfel.radius < epsilon_1e4 /*or contribution == 0.f*/) { continue; }

		coverage += contribution;

		radiance += float4(surfel_radiance, 1.f)
				  * contribution
				  * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()))
				  * gibs_calc_visibility(data, surfel_id, surfel, world_pos);

		if (contribution > 0.f)
		{
			++valid_count;

			if (max_contribution < contribution)
			{
				max_contribution		   = contribution;
				max_contribution_surfel_id = surfel_id;
			}

			if (surfel.frame_since_born() >= surfel_age_max)
			{
				if (surfel.frame_since_born() == surfel_age_max and surfel_id_oldest < surfel_id)
				{
				}
				else
				{
					surfel_id_oldest = surfel_id;
				}

				surfel_age_max = surfel.frame_since_born();
			}
		}
	}


	if (radiance.w == 0.f)
	{
		// radiance_shared = (float4)0;

		const int32_4 cell_idx = gibs_calc_cell_idx(data, lut_data, world_pos);

		const uint32		  cell_idx_flat			= gibs_flatten_cell_idx(data, cell_idx);
		const gibs_cell_entry cell_entry			= gibs_load_cell_entry_arr(data)[cell_idx_flat];
		byte_array<uint32>	  cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_arr(data);

		for (uint32 i = 0; i < /*entry.surfel_count()*/ min(128, cell_entry.surfel_count()); ++i)
		{
			const uint32 surfel_id	  = cell_to_surfel_id_arr[cell_entry.offset + i];
			surfel		 surfel		  = surfel_arr[surfel_id];
			const float	 contribution = gibs_calc_surfel_contribution<false>(data, surfel, world_pos, px_normal);

			assert(surfel.surfel_seen() is_false, g::fmt_gibs, line);

			if (surfel.radius < epsilon_1e4 or contribution == 0.f) { continue; }

			// gibs_set_cell_surfel_ref(data, cell_entry.offset + i);

			radiance += float4(decode_r11g11b10(surfel.radiance_r11g11b10), 1.f)
					  * contribution
					  * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()))
					  * gibs_calc_visibility(data, surfel_id, surfel, world_pos);

			if (contribution > 0.f)
			{
				++valid_count;

				if (max_contribution < contribution)
				{
					max_contribution		   = contribution;
					max_contribution_surfel_id = surfel_id;
				}

				if (surfel.frame_since_born() >= surfel_age_max)
				{
					if (surfel.frame_since_born() == surfel_age_max and surfel_id_oldest < surfel_id)
					{
					}
					else
					{
						surfel_id_oldest = surfel_id;
					}

					surfel_age_max = surfel.frame_since_born();
				}
			}
		}
	}


	assert(cell_entry.offset + cell_entry.surfel_count() <= data.max_surfel_count * 27, g::fmt_forward_plus_gibs_debug_draw_surfels_ps);

	attr_branch()

	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RADIANCE)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			return float4(radiance.xyz / radiance.w, 1);
		}
		else
		{
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_IRRADIANCE)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			const surfel surfel = surfel_arr[max_contribution_surfel_id];
			return float4(decode_r11g11b10(surfel.radiance_r11g11b10), 1.f);
		}
		else
		{
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_VISIBILITY)
	{
		if (surfel_id_oldest != invalid_id_uint32)
		{
			const surfel surfel		= surfel_arr[surfel_id_oldest];
			const float	 visibility = gibs_calc_visibility(data, surfel_id_oldest, surfel, world_pos);
			return float4(random_color(surfel_id_oldest).rgb * visibility, 1.f);
		}
		else
		{
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_INSTABILITY)
	{
		if (surfel_id_oldest != invalid_id_uint32)
		{
			const surfel_msme msme = msme_arr[surfel_id_oldest];
			// float			  spatial_freq = smoothstep(0.f, 1.f, sqrt(max(0.f, msme.incon_var)) * 0.5f);
			// return float4(random_color(surfel_id_oldest).rgb * spatial_freq * (1.f / GIBS_MSME_RD_BLEND), 1);
			// float t = saturate(sqrt(max(0.f, msme.incon_var)) * 10);
			float t = msme.incon_var * (1.f / GIBS_MSME_INCON_BLEND);
			return float4(t, 0.f, 1.f - t, 1.f);
			// return float4(random_color(surfel_id_oldest).rgb * msme.incon_var * (1.f / GIBS_MSME_RD_BLEND), 1);
		}
		else
		{
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RAY_COUNT)
	{
		if (tile_entry.surfel_count() <= 27)
		{
			return float4(tile_entry.surfel_count() / float(27), 0, 0, 1.f);
		}
		else if (tile_entry.surfel_count() <= 128)
		{
			return float4(0, tile_entry.surfel_count() / float(128), 0, 1.f);
		}
		else
		{
			return float4(0, 0, tile_entry.surfel_count() / float(256), 1.f);
		}

		// rw_byte_array<uint32> ray_count_ideal_arr = gibs_load_surfel_ray_count_ideal_rw_arr(data);
		// if (max_contribution_surfel_id != invalid_id_uint32)
		//{
		//	// const surfel surfel = surfel_arr[max_contribution_surfel_id];
		//	// return float4(ray_count_ideal_arr[surfel.alive_idx] / float(GIBS_MAX_RAY_PER_SURFEL), 0, 0, 1);
		// }
		// else
		//{
		//	return color_black;
		// }
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_MSME)
	{
		if (surfel_id_oldest != invalid_id_uint32)
		{
			const surfel_msme msme = msme_arr[tile_to_surfel_id[surfel_id_oldest]];

			return float4(msme.mean_long, 1.f);
		}
		else
		{
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_ID_HASH)
	{
		return float4(random_color(surfel_id_oldest).rgb, 1.f);
		// return float4(random_color(max_contribution_surfel_id) + float3(random_color((px.x / 16) * (px.y / 16)).r, 0, 0), 1.f);
		//  return float4(random_color(surfel_id_smallest).x, surfel_id_smallest, surfel_arr[surfel_id_smallest].radius, 1.f);
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_NORMAL)
	{
		if (surfel_id_oldest == invalid_id_uint32)
		{
			return color_black;
		}
		// return float4(px_normal, 1.f);
		return float4(decode_oct_snorm16(surfel_arr[surfel_id_oldest].normal_oct_snorm16), 1.f);
	}

	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_COVERAGE)
	{
		// return float4(coverage, coverage, coverage, 1.f);
		float ratio = max(0.f, coverage - GIBS_SPAWN_COVERAGE) / float(GIBS_KILL_COVERAGE - GIBS_SPAWN_COVERAGE);
		// if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_AGE)
		//{
		//	return float4(ratio, random_color(cell_idx.x + cell_idx.z).g * 0.5, 1.f - ratio, 1.f);
		// }
		// else
		//{
		//	return float4(ratio, 0, 1.f - ratio, 1.f);

		//	return float4(ratio, random_color(cell_idx.y + cell_idx.z).g, 1.f - ratio, 1.f);
		//}

		if (coverage >= GIBS_KILL_COVERAGE)
		{
			return color_red;
		}
		else if (coverage <= GIBS_SPAWN_COVERAGE)
		{
			return color_blue;
		}

		return float4(0, ratio, 0, 1.f);
		// return float4(ratio, random_color(cell_idx.z).g, 1.f - ratio, 1.f);
	}


	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_AGE)
	{
		// fn_foreach_nbr_cell fn = fn_foreach_nbr_cell::init(world_pos, px_normal, valid_count);
		// gibs_foreach_neighbor_cell(fn, data, lut_data, world_pos);

		// if (fn.contribution_sum > coverage)
		//{
		//	return float4(abs(fn.contribution_sum - coverage), /*fn.size - valid_count*/ 0.f, random_color(cell_idx_flat).b, /*random_color(cell_idx.z).b, */ 1.f);
		// }
		// else
		//{
		//	return float4(0, valid_count - fn.size, random_color(cell_idx_flat).b, 1.f);
		//	// return float4(0, valid_count - fn.size, abs(fn.contribution_sum - coverage), 1.f);
		// }


		if (surfel_id_oldest != invalid_id_uint32)
		{
			uint32 age = surfel_arr[surfel_id_oldest].frame_since_born();
			return float4(age / float(0xff), age / float(0xf), age, 1.f);
		}
	}

	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_CELL)
	{
		return float4(random_color(cell_idx_flat), 1.f);
		return float4(random_color(cell_idx.w), 1.f);
		return float4(random_color(cell_idx.z + cell_idx.w), 1.f);
		return float4(abs(cell_idx.xyz) / float(data.cell_count), 1.f);
		return float4(random_color(cell_idx.z + cell_idx.w).xy, abs(cell_idx.y) / float(data.cell_count), 1.f);
	}

	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_VARIANCE)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			const surfel_msme msme = msme_arr[max_contribution_surfel_id];

			return float4(msme.variance, 1.f);
		}
		else
		{
			return color_black;
		}
	}

	discard;
	return float4(0, 0, 0, 0);
}