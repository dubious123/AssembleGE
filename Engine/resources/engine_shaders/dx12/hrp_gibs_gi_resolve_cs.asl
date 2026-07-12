#include "hrp_common.asli"

// groupshared uint32 gs_scratch[GIBS_SCREEN_GROUP_SHARED_SIZE];
//
// wave_size(32)
//[numthreads(GIBS_SCREEN_TILE_SIZE * GIBS_SCREEN_TILE_SIZE, 1, 1)] void
// main_cs(uint32_3 group_id	   sv_group_id,
//		uint32 group_thread_id sv_group_thread_id)
//
//{
//	bool is_thread_valid = true;
//
//	const int32_2 tile_px = int32_2(group_id.x * GIBS_SCREEN_TILE_SIZE * GIBS_GI_RESOLVE_SCALE, group_id.y * GIBS_SCREEN_TILE_SIZE * GIBS_GI_RESOLVE_SCALE);
//
//	const int32_2 block_px = tile_px + int32_2(group_thread_id % GIBS_SCREEN_TILE_SIZE, group_thread_id / GIBS_SCREEN_TILE_SIZE) * GIBS_GI_RESOLVE_SCALE;
//
//	const int32_2 block_extent = min(int32_2(GIBS_GI_RESOLVE_SCALE, GIBS_GI_RESOLVE_SCALE), int32_2(backbuffer_size) - block_px);
//
//	if (any(block_extent <= 0)) { is_thread_valid = false; }
//
//	const uint32_2 rr_offset = is_thread_valid ? gibs_calc_rr_offset(block_px, uint32_2(backbuffer_size), frame_index) : uint32_2(0, 0);
//
//	const uint32_2 screen_pos_full_res_uint32 = uint32_2(block_px) + rr_offset;
//
//	assert(is_thread_valid is_false or all(screen_pos_full_res_uint32 < uint32_2(backbuffer_size)), g::fmt_gibs_gi_resolve, line, screen_pos_full_res_uint32);
//
//	const gibs_data		  data								   = gibs_load_gibs_data();
//	const gibs_lut_data	  lut_data							   = gibs_load_gibs_lut_data();
//	texture_2d<float>	  depth_tex							   = global_resource_buffer[opaque_depth_buffer_srv_id];
//	texture_2d<uint32_2>  gbuffer							   = global_resource_buffer[opaque_gbuffer_srv_id];
//	rw_texture_2d<float3> gi_resolve_rr_irradiance_curr_buffer = global_resource_buffer[data.h_gi_resolve_rr_irradiance_curr_buffer_uav_id];
//
//	texture_2d<float3> gi_resolve_buffer = global_resource_buffer[data.h_gi_resolve_curr_buffer_srv_id];
//
//	byte_array<gibs_tile_entry> tile_entry_arr		  = gibs_load_tile_entry_arr(data);
//	byte_array<uint32>			tile_to_surfel_id_arr = gibs_load_tile_to_surfel_id_arr(data);
//	rw_byte_array<surfel>		surfel_arr			  = gibs_load_surfel_rw_arr(data);
//	rw_byte_array<surfel_msme>	msme_arr			  = gibs_load_surfel_msme_rw_arr(data);
//
//
//	const float z_depth = is_thread_valid ? load(depth_tex, screen_pos_full_res_uint32.x, screen_pos_full_res_uint32.y, 0) : 0.f;
//
//	if (z_depth == 0.f)
//	{
//		is_thread_valid = false;
//	}
//
//	const uint32 vis_packed			   = is_thread_valid ? gbuffer[screen_pos_full_res_uint32].x : invalid_id_uint32;
//	const uint32 object_id			   = is_thread_valid ? load_opaque_meshlet_render_data(vis_packed & 0x01ffffff).object_id : invalid_id_uint32;
//	const uint32 px_normal_oct_snorm16 = is_thread_valid ? gbuffer[screen_pos_full_res_uint32].y : 0;
//	const float3 px_normal			   = is_thread_valid ? decode_oct_snorm16(px_normal_oct_snorm16) : (float3)0;
//
//	const float2 screen_pos = float2(screen_pos_full_res_uint32 + 0.5f);
//
//	const float2 ndc = screen_to_ndc(screen_pos, inv_backbuffer_size);
//
//	const float4 clip_pos  = mul(view_proj_inv, float4(ndc, z_depth, 1.0));
//	const float3 world_pos = clip_pos.xyz / clip_pos.w;
//
//	const uint32		  tile_id	 = gibs_flatten_tile_idx(data, group_id.xy);
//	const gibs_tile_entry tile_entry = tile_entry_arr[tile_id];
//
//	const uint16 linear_id = uint16(group_thread_id);
//
//	float4 radiance_avg = (float4)0;
//
//	for (uint32 i = linear_id; i < tile_entry.surfel_count() and false; i += GIBS_SCREEN_TILE_SIZE * GIBS_SCREEN_TILE_SIZE)
//	{
//		const uint32 surfel_id	   = tile_to_surfel_id_arr[tile_entry.offset + i];
//		surfel		 surfel		   = surfel_arr[surfel_id];
//		surfel_msme	 msme		   = msme_arr[surfel_id];
//		const float3 surfel_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);
//
//		const uint32_2 surfel_screen_pos = world_to_screen(view_proj, surfel.position, backbuffer_size);
//
//		if (any(group_id.xy != (surfel_screen_pos / (GIBS_SCREEN_TILE_SIZE * GIBS_GI_RESOLVE_SCALE)))) { continue; }
//
//		float4 radiance_shared = (float4)0;
//
//		for (uint32 j = 0; j < /*tile_entry.surfel_count()*/ min(64, tile_entry.surfel_count()); ++j)
//		{
//			const uint32		surfel_nbr_id = tile_to_surfel_id_arr[tile_entry.offset + j];
//			const struct surfel surfel_nbr	  = surfel_arr[surfel_nbr_id];
//
//			if (i == j) { continue; }
//
//			const float contribution = gibs_calc_surfel_contribution<true>(data, surfel_nbr, surfel.position, surfel_normal);
//
//			const float3 surfel_radiance = decode_r11g11b10(surfel_nbr.radiance_r11g11b10);
//
//			const float visibility = gibs_calc_visibility(data, surfel_nbr_id, surfel_nbr, surfel.position);
//
//			if (surfel_nbr.radius < epsilon_1e4 or contribution == 0.f or visibility < 1.f) { continue; }
//
//			radiance_shared += float4(surfel_radiance, 1.f)
//							 * contribution
//							 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel_nbr.frame_since_born()))
//							 * visibility;
//		}
//
//		const float3 surfel_radiance = decode_r11g11b10(surfel.radiance_r11g11b10);
//
//		if (radiance_shared.w > 1.f)
//		{
//			const float blend_factor = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born())) * 0.25;
//			// gibs_update_msme(surfel_radiance * blend_factor + radiance_shared.xyz / radiance_shared.w * (1.f - blend_factor), msme);
//
//			const float t = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()));
//
//			gibs_update_msme(surfel_radiance * blend_factor + radiance_shared.xyz / radiance_shared.w * (1.f - blend_factor), msme /*, lerp(GIBS_MSME_SHORT_WINDOW_BLEND * 10, GIBS_MSME_SHORT_WINDOW_BLEND, t)*/);
//			surfel.radiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
//
//			radiance_avg += radiance_shared;
//		}
//		else
//		{
//			//// const float t = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()));
//			//// gibs_update_msme(surfel_radiance, msme, lerp(GIBS_MSME_SHORT_WINDOW_BLEND * 10, GIBS_MSME_SHORT_WINDOW_BLEND, t));
//			//// surfel.radiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
//			// const float blend_factor = smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born())) * 0.25;
//
//			// radiance_shared += float4(gi_resolve_buffer[screen_pos_full_res_uint32], 1.f);
//
//			// gibs_update_msme(surfel_radiance * blend_factor + radiance_shared.xyz / radiance_shared.w * (1.f - blend_factor), msme /*, lerp(GIBS_MSME_SHORT_WINDOW_BLEND * 10, GIBS_MSME_SHORT_WINDOW_BLEND, t)*/);
//			// surfel.radiance_r11g11b10 = encode_r11g11b10(msme.mean_long);
//
//			// radiance_avg += radiance_shared;
//		}
//
//		msme_arr.store(surfel_id, msme);
//
//		surfel_arr.store(surfel_id, surfel);
//	}
//
//	float coverage = is_thread_valid ? 0.f : float_max;
//
//	float  max_contribution			  = 0.f;
//	uint32 max_contribution_surfel_id = invalid_id_uint32;
//
//	// radiance for new_born, w == weight_sum
//	// float4 radiance		 = (float4)0;
//	float4 radiance_shared = (float4)0;
//
//	for (uint32 i = 0; is_thread_valid and i < min(512, tile_entry.surfel_count()); ++i)
//	{
//		const uint32 surfel_id = tile_to_surfel_id_arr[tile_entry.offset + i];
//		const surfel surfel	   = surfel_arr[surfel_id];
//
//		assert(surfel.surfel_seen(), g::fmt_gibs_gi_resolve, line);
//
//		const float contribution = gibs_calc_surfel_contribution<false>(data, surfel, world_pos, px_normal);
//
//		const float3 surfel_radiance = decode_r11g11b10(surfel.radiance_r11g11b10);
//
//		assert(is_nan(surfel_radiance) is_false, g::fmt_gibs_gi_resolve, line);
//
//		// assert(gibs_load_alive_surfel_id_stack_curr(data)[surfel.alive_idx] == surfel_id, g::fmt_gibs_gi_resolve, line);
//
//		const float fallback_contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);
//
//		if (surfel.radius < epsilon_1e4 or contribution == 0.f) { continue; }
//
//		coverage += contribution;
//
//		radiance_shared += float4(surfel_radiance, 1.f)
//						 * fallback_contribution
//						 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()))
//						 * gibs_calc_visibility(data, surfel_id, surfel, world_pos);
//
//		if (contribution > 0.f and max_contribution < contribution)
//		{
//			max_contribution		   = contribution;
//			max_contribution_surfel_id = surfel_id;
//		}
//	}
//
//	// if (linear_id < GIBS_SCREEN_GROUP_SHARED_SIZE)
//	//{
//	//	gs_scratch[linear_id] = as_uint(float4(0, 0, 0, 0));
//	// }
//
//	// group_memory_barrier_with_sync();
//
//	// if (wave_is_first_lane())
//	//{
//	//	gs_scratch[linear_id / 32] = as_uint(radiance_wave_sum);
//	// }
//
//	// group_memory_barrier_with_sync();
//
//	// const float4 radiance_tile_sum = wave_active_sum(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
//	//													 ? as_float(gs_scratch[wave_lane_index()])
//	//													 : float4(0, 0, 0, 0));
//
//	// const float3 radiance_tile_avg = radiance_tile_sum.w == 0
//	//								   ? color_red.xyz
//	//								   : radiance_tile_sum.xyz / radiance_tile_sum.w;
//
//
//	if (radiance_shared.w < 0.1f)
//	{
//		// radiance_shared = (float4)0;
//
//		const int32_4 cell_idx = gibs_calc_cell_idx(data, lut_data, world_pos);
//
//		const uint32		  cell_idx_flat			= gibs_flatten_cell_idx(data, cell_idx);
//		const gibs_cell_entry cell_entry			= gibs_load_cell_entry_arr(data)[cell_idx_flat];
//		byte_array<uint32>	  cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_arr(data);
//
//		for (uint32 i = 0; i < /*entry.surfel_count()*/ min(128, cell_entry.surfel_count()); ++i)
//		{
//			const uint32 surfel_id	  = cell_to_surfel_id_arr[cell_entry.offset + i];
//			surfel		 surfel		  = surfel_arr[surfel_id];
//			const float	 contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);
//
//			assert(surfel.surfel_seen() is_false, g::fmt_gibs, line);
//
//			const float visibility = gibs_calc_visibility(data, surfel_id, surfel, world_pos);
//
//			if (surfel.radius < epsilon_1e4 or contribution == 0.f /*or visibility != 1.f*/) { continue; }
//
//			radiance_shared += float4(decode_r11g11b10(surfel.radiance_r11g11b10), 1.f)
//							 * contribution
//							 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()))
//							 * visibility;
//		}
//
//		assert(all(radiance_shared >= 0.f) and (is_nan(radiance_shared) is_false), g::fmt_forward_plus_gibs_ray_trace_cs, line, radiance_shared.xyz, radiance_shared.w);
//	}
//
//	// const float4 radiance_wave_sum = wave_active_sum(radiance_shared);
//
//	const float3 new_born_radiance = radiance_shared.w > 0.f
//									   ? radiance_shared.xyz / radiance_shared.w
//									   //: radiance_wave_sum.w > 0
//									   // ? radiance_wave_sum.xyz / radiance_wave_sum.w
//									   : gi_resolve_buffer[screen_pos_full_res_uint32];
//
//	// const float3 new_born_radiance = radiance_shared.w > 0.f
//	//								   ? radiance_shared.xyz / radiance_shared.w
//	//								   : color_red.xyz;
//
//	if (is_thread_valid)
//	{
//		// gi_resolve_rr_irradiance_curr_buffer[screen_pos_full_res_uint32] = radiance_shared.w > 0.f ? radiance_shared.xyz / radiance_shared.w : new_born_radiance;
//		gi_resolve_rr_irradiance_curr_buffer[screen_pos_full_res_uint32] = new_born_radiance;
//		// gi_resolve_rr_geo_curr_buffer[screen_pos_full_res_uint32]		 = uint32_2(px_normal_oct_snorm16, f32tof16(calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth)));
//	}
//
//
//	// todo, apply radiance sharing to max contribution surfel
//
//	if (is_thread_valid)
//	{
//		coverage *= segment_is_opaque_edge(screen_pos_full_res_uint32) ? 0.3f : 1.f;
//	}
//	// min coverage => spawn surfel
//	const uint32 coverage_packed_min = is_thread_valid ? as_uint(f32tof16(coverage)) << 16u | linear_id : invalid_id_uint32;
//
//	// check spawn
//	const uint32 coverage_packed_wave_min = wave_active_min(coverage_packed_min);
//
//	group_memory_barrier_with_sync();
//
//
//	if (wave_is_first_lane())
//	{
//		gs_scratch[linear_id / 32] = coverage_packed_wave_min;
//	}
//
//	group_memory_barrier_with_sync();
//
//	const uint32 coverage_packed_group_min = wave_active_min(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
//																 ? gs_scratch[wave_lane_index()]
//																 : uint32_max);
//
//	const bool need_spawn = coverage < GIBS_SPAWN_COVERAGE
//						and linear_id == uint32_lower_to_uint16(coverage_packed_group_min)
//						/*and cell_entry.count < 128*/
//						and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0);
//
//	// check kill
//	group_memory_barrier_with_sync();
//
//	const uint32 coverage_packed_max = is_thread_valid ? as_uint(f32tof16(coverage)) << 16u | (linear_id + 1) : 0;
//
//	const uint32 coverage_packed_wave_max = wave_active_max(coverage_packed_max);
//
//	if (wave_is_first_lane())
//	{
//		gs_scratch[linear_id / 32] = coverage_packed_wave_max;
//	}
//
//	group_memory_barrier_with_sync();
//
//	const uint32 coverage_packed_group_max = wave_active_max(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
//																 ? gs_scratch[wave_lane_index()]
//																 : 0u);
//
//	const bool need_kill = (coverage > GIBS_KILL_COVERAGE /*or cell_entry.count > 128*/)
//					   and linear_id == (uint32_lower_to_uint16(coverage_packed_group_max) - 1)
//					   and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0);
//
//
//	const float rnd = random_pcg3d(uint32_3(screen_pos_full_res_uint32, frame_index)).x;
//	if (need_spawn)
//	{
//		const float spawn_prob = /*coverage == 0.f
//								   ? 1.f
//								   : */
//			(GIBS_SPAWN_COVERAGE - coverage) / float(GIBS_SPAWN_COVERAGE)
//			/* px_world_area(dispatch_thread_id.xy, world_pos - camera_pos, px_normal, camera_forward, tan_fov_y_half, backbuffer_size.y)*/
//			// * (1.f + dead_stack.size() / float(data.max_surfel_count))
//			* (coverage == 0.f ? 1000 : 1)
//			* GIBS_SPAWN_PROB_FACTOR;
//
//		// if (coverage == 0.f)
//		//{
//		//	debug_log(g::fmt_gibs_gi_resolve, coverage, spawn_prob);
//		// }
//
//		if (rnd < spawn_prob)
//		{
//			const object_data obj = load_object_data(object_id);
//
//			gibs_set_tile_surfel_spawn(data,
//									   tile_id,
//									   object_id,
//									   rotate_inv(obj.quaternion, world_pos - obj.pos) / obj.scale,
//									   encode_oct_snorm16(normalize(rotate_inv(obj.quaternion, px_normal) * obj.scale)),
//									   encode_r11g11b10(new_born_radiance));
//		}
//	}
//	else if (need_kill)
//	{
//		const float kill_prob = coverage / float(GIBS_KILL_COVERAGE)
//							  //* (0.01)
//							  // * calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth) / (cam_far_z - cam_near_z)
//							  // * (1.f + alive_stack.size() / float(data.max_surfel_count))
//							  * GIBS_KILL_PROB_FACTOR;
//		if (rnd < kill_prob)
//		{
//			gibs_set_tile_surfel_kill(data, tile_id, max_contribution_surfel_id);
//
//			// if (cell_entry.count > 128 /*and false*/)
//			//{
//			//	gibs_load_surfel_spawn_kill_stack(data).push(surfel_spawn_data::init_kill(cell_to_surfel_id[cell_entry.offset + cell_entry.count - 1]));
//			// }
//			// else
//			//{
//			//	gibs_load_surfel_spawn_kill_stack(data).push(surfel_spawn_data::init_kill(max_contribution_surfel_id));
//			// }
//		}
//	}
// }

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32_3 group_id	   sv_group_id,
		uint32 group_thread_id sv_group_thread_id)

{
	const gibs_data				data				  = gibs_load_gibs_data();
	const gibs_lut_data			lut_data			  = gibs_load_gibs_lut_data();
	texture_2d<float>			depth_tex			  = global_resource_buffer[opaque_depth_buffer_srv_id];
	texture_2d<uint32_2>		gbuffer				  = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float3>			gi_resolve_buffer	  = global_resource_buffer[data.h_gi_resolve_curr_buffer_srv_id];
	byte_array<gibs_tile_entry> tile_entry_arr		  = gibs_load_tile_entry_arr(data);
	byte_array<uint32>			tile_to_surfel_id_arr = gibs_load_tile_to_surfel_id_arr(data);
	rw_byte_array<surfel>		surfel_arr			  = gibs_load_surfel_rw_arr(data);
	rw_byte_array<surfel_msme>	msme_arr			  = gibs_load_surfel_msme_rw_arr(data);

	rw_array<gibs_gi_resolve_cell> gi_resolve_cell_arr = gibs_load_gi_resolve_cell_rw_arr(data);

	bool is_thread_valid = true;

	const int32_2 tile_px = int32_2(group_id.x * GIBS_SCREEN_TILE_SIZE * GIBS_GI_RESOLVE_SCALE, group_id.y * GIBS_SCREEN_TILE_SIZE * GIBS_GI_RESOLVE_SCALE);

	const int32_2 extent = int32_2(backbuffer_size);

	const int32	  gi_resolve_cell_id		= group_thread_id / GIBS_GI_RESOLVE_SAMPLES_PER_CELL;
	const int32_2 gi_resolve_local_cell_idx = int32_2(gi_resolve_cell_id % GIBS_GI_RESOLVE_CELL_DIM, gi_resolve_cell_id / GIBS_GI_RESOLVE_CELL_DIM);

	const int32_2 px = tile_px
					 + gi_resolve_local_cell_idx * GIBS_GI_RESOLVE_CELL_SIZE
					 + int32_2(random_r2_sequence(group_thread_id + (frame_index & 0x00ffffff) * AGE_WAVE_SIZE) * float(GIBS_GI_RESOLVE_CELL_SIZE));

	if (any(px >= extent)) { is_thread_valid = false; }

	const float z_depth = is_thread_valid ? depth_tex[px] : 0.f;

	if (z_depth == 0.f)
	{
		is_thread_valid = false;
	}

	const uint32 vis_packed			   = is_thread_valid ? gbuffer[px].x : invalid_id_uint32;
	const uint32 object_id			   = is_thread_valid ? load_opaque_meshlet_render_data(vis_packed & 0x01ffffff).object_id : invalid_id_uint32;
	const uint32 px_normal_oct_snorm16 = is_thread_valid ? gbuffer[px].y : 0;
	const float3 px_normal			   = is_thread_valid ? decode_oct_snorm16(px_normal_oct_snorm16) : (float3)0;

	const float2 screen_pos = float2(px + 0.5f);
	const float2 ndc		= screen_to_ndc(screen_pos, inv_backbuffer_size);
	const float4 clip_pos	= mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	const float3 world_pos	= clip_pos.xyz / clip_pos.w;

	const uint32		  tile_id	 = gibs_flatten_tile_idx(data, group_id.xy);
	const gibs_tile_entry tile_entry = tile_entry_arr[tile_id];

	const uint16 linear_id = uint16(group_thread_id);

	float  coverage					  = is_thread_valid ? 0.f : float_max;
	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;

	float4 radiance_shared = (float4)0;

	for (uint32 i = 0; is_thread_valid and i < min(512, tile_entry.surfel_count()); ++i)
	{
		const uint32 surfel_id = tile_to_surfel_id_arr[tile_entry.offset + i];
		const surfel surfel	   = surfel_arr[surfel_id];

		const float contribution = gibs_calc_surfel_contribution<false>(data, surfel, world_pos, px_normal);

		const float3 surfel_radiance = decode_r11g11b10(surfel.radiance_r11g11b10);

		// assert(is_nan(surfel_radiance) is_false, g::fmt_gibs_gi_resolve, line);

		// assert(gibs_load_alive_surfel_id_stack_curr(data)[surfel.alive_idx] == surfel_id, g::fmt_gibs_gi_resolve, line);

		const float fallback_contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);

		if (surfel.radius < epsilon_1e4 or contribution == 0.f) { continue; }

		coverage += contribution;

		radiance_shared += float4(surfel_radiance, 1.f)
						 * fallback_contribution
						 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()))
						 * gibs_calc_visibility(data, surfel_id, surfel, world_pos);

		if (contribution > 0.f and max_contribution < contribution)
		{
			max_contribution		   = contribution;
			max_contribution_surfel_id = surfel_id;
		}
	}

	if (is_thread_valid and radiance_shared.w < 0.1f)
	{
		const int32_4 cell_idx = gibs_calc_cell_idx(data, lut_data, world_pos);

		const uint32		  cell_idx_flat			= gibs_flatten_cell_idx(data, cell_idx);
		const gibs_cell_entry cell_entry			= gibs_load_cell_entry_arr(data)[cell_idx_flat];
		byte_array<uint32>	  cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_arr(data);

		for (uint32 i = 0; i < /*entry.surfel_count()*/ min(128, cell_entry.surfel_count()); ++i)
		{
			const uint32 surfel_id	  = cell_to_surfel_id_arr[cell_entry.offset + i];
			surfel		 surfel		  = surfel_arr[surfel_id];
			const float	 contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);

			assert(surfel.surfel_seen() is_false, g::fmt_gibs, line);

			const float visibility = gibs_calc_visibility(data, surfel_id, surfel, world_pos);

			if (surfel.radius < epsilon_1e4 or contribution == 0.f /*or visibility != 1.f*/) { continue; }

			radiance_shared += float4(decode_r11g11b10(surfel.radiance_r11g11b10), 1.f)
							 * contribution
							 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(surfel.frame_since_born()))
							 * visibility;
		}
	}

	const float3 new_born_radiance = radiance_shared.w > 0.f
									   ? radiance_shared.xyz / radiance_shared.w
									   //: radiance_wave_sum.w > 0
									   // ? radiance_wave_sum.xyz / radiance_wave_sum.w
									   : gi_resolve_buffer[px];

	// const float3 new_born_radiance = radiance_shared.w > 0.f
	//								   ? radiance_shared.xyz / radiance_shared.w
	//								   : color_red.xyz;


	const int32_2 gi_resolve_global_cell_idx = int32_2(group_id.xy) * GIBS_GI_RESOLVE_CELL_DIM + int32_2(gi_resolve_local_cell_idx);
	const uint32  gi_resolve_global_cell_id	 = gi_resolve_global_cell_idx.y * data.tile_count_w * GIBS_GI_RESOLVE_CELL_DIM + gi_resolve_global_cell_idx.x;
	const uint32  idx						 = gi_resolve_global_cell_id * GIBS_GI_RESOLVE_SAMPLES_PER_CELL + (group_thread_id % GIBS_GI_RESOLVE_SAMPLES_PER_CELL);

	gibs_gi_resolve_cell res;
	res.coverage			 = cast<half>(is_thread_valid ? coverage : 0.f);
	res.irradiance_r11g11b10 = encode_r11g11b10(new_born_radiance);
	res.normal_oct_snorm16	 = px_normal_oct_snorm16;

	const uint32_2 rel = uint32_2(px - gi_resolve_global_cell_idx * GIBS_GI_RESOLVE_CELL_SIZE);
	res.sample_px	   = uint16((rel.x & 0xff) | ((rel.y & 0xff) << 8u));
	res.z_lin		   = calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth);

	gi_resolve_cell_arr.store(idx, res);

	if (is_thread_valid)
	{
		// gi_resolve_buffer[px]  = new_born_radiance;
		coverage *= segment_is_opaque_edge(px) ? 0.1f : 1.f;
	}

	const uint32 coverage_packed_min = is_thread_valid ? as_uint(f32tof16(coverage)) << 16u | linear_id : invalid_id_uint32;

	const uint32 coverage_packed_wave_min = wave_active_min(coverage_packed_min);

	const bool need_spawn = coverage < GIBS_SPAWN_COVERAGE
						and linear_id == uint32_lower_to_uint16(coverage_packed_wave_min)
						/*and cell_entry.count < 128*/
						and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0);

	const uint32 coverage_packed_max = is_thread_valid ? as_uint(f32tof16(coverage)) << 16u | (linear_id + 1) : 0;

	const uint32 coverage_packed_wave_max = wave_active_max(coverage_packed_max);

	const bool need_kill = (coverage > GIBS_KILL_COVERAGE /*or cell_entry.count > 128*/)
					   and linear_id == (uint32_lower_to_uint16(coverage_packed_wave_max) - 1)
					   and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0);


	const float prob = random_pcg3d(uint32_3(px, frame_index)).x;
	if (need_spawn)
	{
		const float spawn_prob = /*coverage == 0.f
								   ? 1.f
								   : */
			(GIBS_SPAWN_COVERAGE - coverage) / float(GIBS_SPAWN_COVERAGE)
			/* px_world_area(dispatch_thread_id.xy, world_pos - camera_pos, px_normal, camera_forward, tan_fov_y_half, backbuffer_size.y)*/
			// * (1.f + dead_stack.size() / float(data.max_surfel_count))
			* (coverage == 0.f ? 1000 : 1)
			* GIBS_SPAWN_PROB_FACTOR;

		if (prob < spawn_prob)
		{
			const object_data obj = load_object_data(object_id);

			gibs_set_tile_surfel_spawn(data,
									   tile_id,
									   object_id,
									   rotate_inv(obj.quaternion, world_pos - obj.pos) / obj.scale,
									   encode_oct_snorm16(normalize(rotate_inv(obj.quaternion, px_normal) * obj.scale)),
									   encode_r11g11b10(new_born_radiance));
		}
	}
	else if (need_kill)
	{
		const float kill_prob = coverage / float(GIBS_KILL_COVERAGE)
							  //* (0.01)
							  // * calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth) / (cam_far_z - cam_near_z)
							  // * (1.f + alive_stack.size() / float(data.max_surfel_count))
							  * GIBS_KILL_PROB_FACTOR;
		if (prob < kill_prob)
		{
			gibs_set_tile_surfel_kill(data, tile_id, max_contribution_surfel_id);
		}
	}
}