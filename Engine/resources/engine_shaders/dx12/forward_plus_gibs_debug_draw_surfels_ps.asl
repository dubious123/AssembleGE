#include "forward_plus_common.asli"

float4
main_ps(float4 pos sv_position) sv_target_0
{
	const gibs_data							 data			   = gibs_load_gibs_data();
	const gibs_lut_data						 lut_data		   = gibs_load_gibs_lut_data();
	const texture_2d<float>					 depth_tex		   = global_resource_buffer[depth_buffer_texture_id];
	const texture_2d<uint32_2>				 gbuffer		   = global_resource_buffer[data.h_gbuffer_srv_id];
	const texture_2d<float2>				 visibility_atlas  = global_resource_buffer[data.h_visibility_atlas_srv_id];
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
		discard;
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

	float  min_contribution = float_max;
	uint32 min_contribution_surfel_id;

	uint32 surfel_id_largest = 0;

	// radiance for new_born, w == weight_sum
	float4 radiance = (float4)0;
	float  coverage = 0.f;
	for (uint32 i = 0; i < cell_entry.count; ++i)
	{
		const uint32			  surfel_id		= cell_to_surfel_id[cell_entry.offset + i];
		const surfel			  surfel		= surfel_arr[surfel_id];
		const surfel_recycle_data recycle		= recycle_arr[surfel_id];
		const float				  contribution	= gibs_calc_surfel_contribution(data, surfel, world_pos, px_normal);
		coverage							   += contribution;

		// todo, change to msme instability
		const float3  rel = world_pos - surfel.position;
		const int32_2 px  = gibs_calc_atlas_tile_px(gibs_calc_atlas_offset(data, surfel_id), normalize(rel));

		radiance += float4(surfel.radiance, 1.f)
				  * contribution
				  * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born))
				  * gibs_clac_visibility(visibility_atlas[px], saturate(length(rel) / surfel.radius));

		coverage += contribution;
		if (min_contribution > contribution)
		{
			min_contribution		   = contribution;
			min_contribution_surfel_id = surfel_id;
		}

		if (surfel_id > surfel_id_largest)
		{
			surfel_id_largest = surfel_id;
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RADIANCE)
	{
		return radiance;
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_IRRADIANCE)
	{
		return float4(cell_idx / 64.f);
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_VISIBILITY)
	{
		if (cell_idx.w == 0)
		{
			return float4(0, 0, 0, 1);
		}
		else
		{
			return float4(cell_idx.z / 16.f, 0, 0, 1);
		}
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_INSTABILITY)
	{
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RAY_COUNT)
	{
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_MSME)
	{
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_ID_HASH)
	{
		return float4(random_color(surfel_id_largest).x, surfel_id_largest, surfel_arr[surfel_id_largest].radius, 1.f);
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_NORMAL)
	{
		return float4(decode_oct_snorm16(surfel_arr[surfel_id_largest].normal_oct_snorm16), 1.f);
	}
	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_CELL_OCCUPANCY)
	{
		return float4(cell_entry.count, 0.f, 0.f, 1.f);
		// return float4(coverage, cell_entry.count / 10.f, 0.f, 1.f);
	}

	if (data.debug_flags & GIBS_DEBUG_FLAGS_RENDER_RADIANCE)
	{
	}


	// if (data.debug_flags | GIBS_DEBUG_FLAGS_RENDER_ID_HASH)
	//{

	//}
	discard;

	return float4(0, 0, 0, 0);
}