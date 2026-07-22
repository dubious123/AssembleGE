#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(GIBS_GI_RESOLVE_SAMPLE_PER_TILE, 1, 1)] void
main_cs(uint32_3 group_id	   sv_group_id,
		uint32 group_thread_id sv_group_thread_id)

{
	const gibs_data						data				  = gibs::load_data();
	const gibs_lut_data					lut_data			  = gibs::load_lut_data();
	texture_2d<float>					depth_tex			  = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2>				gbuffer				  = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float3>					gi_resolve_buffer	  = global_resource_buffer[data.h_gi_resolve_curr_buffer_srv_id];
	structured_buffer<gibs_tile_surfel> surfel_buffer		  = global_resource_buffer[data.h_tile_surfel_buffer_srv_id];
	byte_array<gibs_tile_surfel_entry>	tile_entry_arr		  = gibs::tile::surfel_entry_arr(data);
	byte_array<uint32>					tile_to_surfel_id_arr = gibs::tile::tile_to_surfel_id_arr(data);

	byte_array<uint16>					 gi_resolve_sample_pos_arr	  = gibs::gi_resolve_sample_pos_arr(data);
	rw_array<gibs_gi_resolve_sample_res> gi_resolve_sample_res_rw_arr = gibs::gi_resolve_sample_res_rw_arr(data);


	const int32_2 tile_idx = group_id.xy;
	const int32_2 tile_px  = int32_2(tile_idx * GIBS_GI_RESOLVE_TILE_SIZE);

	const int32	  block_id	= group_thread_id / (GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK);
	const int32_2 block_idx = tile_idx * GIBS_GI_RESOLVE_BLOCK_DIM
							+ int32_2(block_id % GIBS_GI_RESOLVE_BLOCK_DIM, block_id / GIBS_GI_RESOLVE_BLOCK_DIM);

	const int32_2 block_px = block_idx * GIBS_GI_RESOLVE_BLOCK_SIZE;

	const uint32 sample_id = block_idx.y * data.tile_count_w * GIBS_GI_RESOLVE_BLOCK_DIM * GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK
						   + block_idx.x * GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK
						   + group_thread_id % GIBS_GI_RESOLVE_SAMPLE_PER_BLOCK;

	const uint16 sample_pos_lin = gi_resolve_sample_pos_arr[sample_id];

	const int32_2 px = block_px
					 + int32_2(sample_pos_lin % GIBS_GI_RESOLVE_BLOCK_SIZE, sample_pos_lin / GIBS_GI_RESOLVE_BLOCK_SIZE);

	const int32_2 extent = int32_2(backbuffer_size);

	if (sample_pos_lin == 0xffffu)
	{
		gibs_gi_resolve_sample_res sample_res = gi_resolve_sample_res_rw_arr[sample_id];
		sample_res.coverage					  = 0.h;
		gi_resolve_sample_res_rw_arr.store(sample_id, sample_res);
		return;
	}

	// if (any(px >= extent)) { is_thread_valid = false; }

	const float z_depth = depth_tex[px];

	// if (z_depth == 0.f)
	//{
	//	is_thread_valid = false;
	// }

	const uint32 vis_packed			   = gbuffer[px].x;
	const uint32 object_id			   = load_opaque_meshlet_render_data(vis_packed & 0x01ffffff).object_id;
	const uint32 px_normal_oct_snorm16 = gbuffer[px].y;
	const float3 px_normal			   = decode_oct_snorm16(px_normal_oct_snorm16);

	const float2 screen_pos = float2(px + 0.5f);
	const float2 ndc		= screen_to_ndc(screen_pos, inv_backbuffer_size);
	const float4 clip_pos	= mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	const float3 world_pos	= clip_pos.xyz / clip_pos.w;

	const uint32				 tile_id	= gibs::tile::calc_id(data, group_id.xy);
	const gibs_tile_surfel_entry tile_entry = tile_entry_arr[tile_id];

	float  coverage					  = 0.f;
	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;

	float4 irradiance_sum = zero<float4>();

	for (uint32 i = 0; i < min(512, tile_entry.surfel_count); ++i)
	{
		const uint32		   surfel_id = tile_to_surfel_id_arr[tile_entry.offset + i];
		const gibs_tile_surfel surfel	 = surfel_buffer[surfel_id];

		const float contribution = gibs::calc_tile_surfel_contribution<false>(data, surfel, world_pos, px_normal);

		const float3 surfel_irradiance = decode_r11g11b10(surfel.irradiance_r11g11b10);

		if (contribution == 0.f) { continue; }

		const float visibility = gibs::calc_surfel_visibility<true, gibs_tile_surfel>(data, surfel_id, surfel, world_pos);

		const float contribution_vis = contribution * visibility;

		coverage += contribution_vis;

		irradiance_sum += float4(surfel_irradiance, 1.f)
						* contribution_vis
						* smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.recycle_data.frame_since_born()));

		if (max_contribution < contribution_vis)
		{
			max_contribution		   = contribution_vis;
			max_contribution_surfel_id = surfel_id;
		}
	}

	if (irradiance_sum.w < 0.1f)
	{
		const float4 fallback  = gibs::sample_irradiance(data, world_pos, px_normal);
		irradiance_sum		  += float4(fallback.xyz * fallback.w, fallback.w);
	}

	const float3 new_born_irradiance = irradiance_sum.w > 0.f
										 ? irradiance_sum.xyz / irradiance_sum.w
										 //: radiance_wave_sum.w > 0
										 // ? radiance_wave_sum.xyz / radiance_wave_sum.w
										 : gi_resolve_buffer[px];

	// const float3 new_born_radiance = radiance_shared.w > 0.f
	//								   ? radiance_shared.xyz / radiance_shared.w
	//								   : color_red.xyz;

	gibs_gi_resolve_sample_res sample_res;
	sample_res.coverage				= cast<half>(coverage);
	sample_res.irradiance_r11g11b10 = encode_r11g11b10(new_born_irradiance);
	sample_res.normal_oct_snorm16	= px_normal_oct_snorm16;
	sample_res.sample_pos_lin		= sample_pos_lin;
	sample_res.z_lin				= calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth);

	gi_resolve_sample_res_rw_arr.store(sample_id, sample_res);

	const bool need_spawn = coverage < GIBS_TILE_SURFEL_SPAWN_COVERAGE
						and (gibs::debug::freeze_spawn_kill(data) is_false);

	const bool need_kill = coverage > GIBS_TILE_SURFEL_KILL_COVERAGE
					   and (gibs::debug::freeze_spawn_kill(data) is_false);

	const float prob = random_pcg3d(uint32_3(px, frame_index)).x;
	if (need_spawn)
	{
		const float spawn_prob = /*coverage == 0.f
								   ? 1.f
								   : */
			(GIBS_TILE_SURFEL_SPAWN_COVERAGE - coverage) / float(GIBS_TILE_SURFEL_SPAWN_COVERAGE)
			/** px_world_area(px, world_pos - camera_pos, px_normal, camera_forward, tan_fov_y_half, backbuffer_size.y)*/
			// * (1.f + dead_stack.size() / float(data.max_surfel_count))
			* (coverage == 0.f ? 1000 : 1)
			* GIBS_SPAWN_PROB_FACTOR;

		if (prob < spawn_prob)
		{
			const object_data obj = load_object_data(object_id);

			gibs::tile::set_surfel_spawn(data,
										 sample_id,
										 object_id,
										 rotate_inv(obj.quaternion, world_pos - obj.pos) / obj.scale,
										 encode_oct_snorm16(normalize(rotate_inv(obj.quaternion, px_normal) * obj.scale)),
										 encode_r11g11b10(new_born_irradiance));
		}
	}
	else if (need_kill)
	{
		const float kill_prob = coverage / float(GIBS_TILE_SURFEL_KILL_COVERAGE)
							  //* (0.01)
							  // * calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth) / (cam_far_z - cam_near_z)
							  // * (1.f + alive_stack.size() / float(data.max_surfel_count))
							  * (segment_is_opaque_edge(px) ? 0.1f : 1.f)
							  * GIBS_KILL_PROB_FACTOR;
		if (prob < kill_prob)
		{
			gibs::tile::set_surfel_kill(data, sample_id, max_contribution_surfel_id);
		}
	}
}