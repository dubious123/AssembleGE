#include "forward_plus_common.asli"
#define max_surfel_per_cell 1000

struct fn_foreach_nbr_cell
{
	float3 world_pos;
	float3 px_normal;

	uint32 surfel_id_arr[max_surfel_per_cell];
	uint32 size;

	int32_4 cell_idx_arr[27];
	uint32	cell_idx_size;


	float contribution_sum;

	uint32 valid_count;

	static fn_foreach_nbr_cell
	init(float3 world_pos, float3 px_normal, uint32 valid_count)
	{
		fn_foreach_nbr_cell res;

		res.contribution_sum = 0.f;
		res.size			 = 0u;
		res.world_pos		 = world_pos;
		res.px_normal		 = px_normal;

		res.cell_idx_size = 0u;

		res.valid_count = valid_count;

		assert(valid_count <= max_surfel_per_cell);

		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		const uint32					  cell_idx_flat		= gibs_flatten_cell_idx(data, cell_idx);
		const byte_array<uint32>		  cell_to_surfel_id = gibs_load_cell_to_surfel_id_arr(data);
		const byte_array<gibs_cell_entry> cell_entry_arr	= gibs_load_cell_entry_arr(data);
		const rw_array<surfel>			  surfel_arr		= gibs_load_surfel_rw_arr(data);
		const gibs_cell_entry			  cell_entry		= cell_entry_arr[cell_idx_flat];


		for (uint32 i = 0; i < cell_idx_size; ++i)
		{
			assert(all(cell_idx == cell_idx_arr[i]) is_false);
			assert(cell_idx_flat != gibs_flatten_cell_idx(data, cell_idx_arr[i]));
		}

		cell_idx_arr[cell_idx_size++] = cell_idx;

		for (uint32 i = 0; i < cell_entry.count; ++i)
		{
			const uint32 surfel_id	  = cell_to_surfel_id[cell_entry.offset + i];
			const surfel surfel		  = surfel_arr[surfel_id];
			const float	 contribution = gibs_calc_surfel_contribution(data, surfel, world_pos, px_normal);

			assert(contribution >= 0.f and contribution <= 1.f);

			if (contribution == 0.f) { continue; }

			bool found = false;
			for (uint32 j = 0; j < size; ++j)
			{
				if (surfel_id_arr[j] == surfel_id)
				{
					found = true;
					break;
				}
			}

			if (found is_false)
			{
				contribution_sum += contribution;

				surfel_id_arr[size++] = surfel_id;

				assert(size <= max_surfel_per_cell);
			}
		}

		assert(size <= valid_count);
	}
};

float4
main_ps(float4 pos sv_position) sv_target_0
{
	const gibs_data							 data			   = gibs_load_gibs_data();
	const gibs_lut_data						 lut_data		   = gibs_load_gibs_lut_data();
	const texture_2d<float>					 depth_tex		   = global_resource_buffer[depth_buffer_texture_id];
	const texture_2d<uint32_2>				 gbuffer		   = global_resource_buffer[data.h_gbuffer_srv_id];
	const texture_2d<float2>				 visibility_atlas  = global_resource_buffer[data.h_visibility_atlas_srv_id];
	const texture_2d<float2>				 luminance_atlas   = global_resource_buffer[data.h_irradiance_atlas_srv_id];
	const byte_array<gibs_cell_entry>		 cell_entry_arr	   = gibs_load_cell_entry_arr(data);
	const byte_array<uint32>				 cell_to_surfel_id = gibs_load_cell_to_surfel_id_arr(data);
	const rw_array<surfel>					 surfel_arr		   = gibs_load_surfel_rw_arr(data);
	const rw_byte_array<surfel_geometry>	 geo_arr		   = gibs_load_surfel_geometry_rw_arr(data);
	const rw_byte_array<surfel_recycle_data> recycle_arr	   = gibs_load_surfel_recycle_data_rw_arr(data);
	const rw_byte_array<surfel_msme>		 msme_arr		   = gibs_load_surfel_msme_rw_arr(data);

	uint32_2 px = uint32_2(pos.xy);

	float2 uv = pos.xy * inv_backbuffer_size;

	const float z_depth = load(depth_tex, px.x, px.y, 0);

	if (z_depth == 0.f)
	{
		return float4(0, 0, 0, 0);
	}

	const uint32 object_id			   = load(gbuffer, px.x, px.y, 0).x;
	const uint32 px_normal_oct_snorm16 = load(gbuffer, px.x, px.y, 0).y;
	const float3 px_normal			   = decode_oct_snorm16(px_normal_oct_snorm16);

	float2 ndc = screen_to_ndc(pos.xy, inv_backbuffer_size);

	const float4 clip_pos  = mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	const float3 world_pos = clip_pos.xyz / clip_pos.w;

	const int32_4 cell_idx		= gibs_calc_cell_idx(data, lut_data, world_pos);
	const uint32  cell_idx_flat = gibs_flatten_cell_idx(data, cell_idx);

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

	assert(cell_entry.offset + cell_entry.count <= data.max_surfel_count * 27);

	int32 valid_count = 0;
	for (uint32 i = 0; i < cell_entry.count; ++i)
	{
		const uint32			  surfel_id = cell_to_surfel_id[cell_entry.offset + i];
		const surfel			  surfel	= surfel_arr[surfel_id];
		const surfel_recycle_data recycle	= recycle_arr[surfel_id];

		if (surfel.radius == 0.f) { continue; }

		const float contribution  = gibs_calc_surfel_contribution(data, surfel, world_pos, px_normal);
		coverage				 += contribution;
		const float cell_size	  = gibs_calc_cell_size(data, lut_data, surfel);
		// assert(surfel.radius <= cell_size);

		radiance += float4(surfel.radiance, 1.f)
				  * contribution
				  * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born))
				  * gibs_calc_visibility(data, surfel_id, surfel, world_pos);

		if (contribution > 0.f)
		{
			++valid_count;
			if (min_contribution > contribution)
			{
				min_contribution		   = contribution;
				min_contribution_surfel_id = surfel_id;
			}

			if (max_contribution < contribution)
			{
				max_contribution		   = contribution;
				max_contribution_surfel_id = surfel_id;
			}

			if (length(world_pos - surfel.position) < surfel_dist_min)
			{
				surfel_dist_min	  = length(world_pos - surfel.position);
				surfel_id_closest = surfel_id;
			}

			if (surfel.radius >= surfel_radius_max)
			{
				if (surfel.radius == surfel_radius_max)
				{
					if (length(world_pos - surfel.position) > length(world_pos - surfel_arr[surfel_id_largest].position))
					{
						surfel_radius_max = surfel.radius;
						surfel_id_largest = surfel_id;
					}
				}
				else
				{
					surfel_radius_max = surfel.radius;
					surfel_id_largest = surfel_id;
				}
			}

			if (surfel_id < surfel_id_smallest)
			{
				surfel_id_smallest = surfel_id;
			}

			if (recycle.frame_since_born > surfel_age_max)
			{
				surfel_id_oldest = surfel_id;
				surfel_age_max	 = recycle.frame_since_born;
			}
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RADIANCE)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			return radiance;
			return float4(surfel_arr[max_contribution_surfel_id].radiance, 1.f);
		}
		else
		{
			discard;
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_IRRADIANCE)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			uint32_2 atlas_offset = gibs_calc_atlas_offset(data, max_contribution_surfel_id);
			return float4(luminance_atlas[atlas_offset + uint32_2(3, 3)].y, 0, 0, 1);
		}
		else
		{
			discard;
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_VISIBILITY)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			uint32_2 atlas_offset = gibs_calc_atlas_offset(data, max_contribution_surfel_id);
			return float4(visibility_atlas[atlas_offset + uint32_2(3, 3)].y, 0, 0, 1);
		}
		else
		{
			discard;
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_INSTABILITY)
	{
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RAY_COUNT)
	{
		// if (max_contribution_surfel_id != invalid_id_uint32)
		//{
		//	return float4(visibility_atlas[atlas_offset + uint32_2(5, 5)].y, 0, 0, 1);
		// }
		// else
		//{
		//	return color_black;
		// }
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_MSME)
	{
		if (max_contribution_surfel_id != invalid_id_uint32)
		{
			const surfel_msme msme = msme_arr[max_contribution_surfel_id];

			return float4(msme.mean_long, 1.f);
		}
		else
		{
			discard;
			return color_black;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_ID_HASH)
	{
		// if (cell_entry.count == 0)
		//{
		//	return color_red;
		// }
		// else
		//{
		//	return float4(cell_entry.count / float(0xff), 0, 0, 1);
		//	return float4(0, float(valid_count) / cell_entry.count * 2, 1.f - float(valid_count) / cell_entry.count, 1);
		// }
		//  if (cell_entry.count == 0)
		//{
		//	return color_green;
		//  }
		//  if (min_contribution_surfel_id == invalid_id_uint32)
		//{
		//	return color_blue;
		//  }
		//  if (surfel_arr[min_contribution_surfel_id].radius == 0.f)
		//{
		//	return color_red;
		//  }

		// return float4(random_color(min_contribution_surfel_id).x, min_contribution, surfel_arr[min_contribution_surfel_id].radius, 1.f);
		// return float4(random_color(min_contribution_surfel_id).xy, surfel_arr[min_contribution_surfel_id].radius, 1.f);
		// return float4(random_color(min_contribution_surfel_id), 1.f);


		// if (cell_entry.count == 0 or max_contribution_surfel_id == invalid_id_uint32)
		//{
		//	return color_red;
		// }
		// return float4(random_color(max_contribution_surfel_id).x, max_contribution, surfel_arr[max_contribution_surfel_id].radius, 1.f);
		// return float4(random_color(max_contribution_surfel_id).xy, surfel_arr[max_contribution_surfel_id].radius, 1.f);
		return float4(random_color(max_contribution_surfel_id) + float3(random_color((px.x / 16) * (px.y / 16)).r, 0, 0), 1.f);
		// return float4(random_color(surfel_id_smallest).x, surfel_id_smallest, surfel_arr[surfel_id_smallest].radius, 1.f);
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_NORMAL)
	{
		if (cell_entry.count == 0 or surfel_id_smallest == invalid_id_uint32)
		{
			return color_red;
		}
		// return float4(px_normal, 1.f);
		return float4(decode_oct_snorm16(surfel_arr[surfel_id_smallest].normal_oct_snorm16), 1.f);
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

		return float4(ratio, random_color(cell_idx.z).g, 1.f - ratio, 1.f);
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
			uint32 age = recycle_arr[surfel_id_oldest].frame_since_born;
			return float4(age / float(0xffff), age / float(0xfff), age / float(0xff), 1.f);
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

	return float4(0, 0, 0, 0);
}