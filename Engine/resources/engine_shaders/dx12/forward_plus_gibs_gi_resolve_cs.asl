#include "forward_plus_common.asli"

groupshared uint32 gs_scratch[GIBS_SCREEN_GROUP_SHARED_SIZE];

wave_size(32)
[numthreads(GIBS_SCREEN_TILE_SIZE, GIBS_SCREEN_TILE_SIZE, 1)] void
main_cs(uint32_3 group_thread_id	sv_group_thread_id,
		uint32_3 dispatch_thread_id sv_dispatch_thread_id)

{
	bool is_thread_valid = true;
	if (dispatch_thread_id.x >= (uint32)backbuffer_size.x or dispatch_thread_id.y >= (uint32)backbuffer_size.y)
	{
		is_thread_valid = false;
	}

	const gibs_data				data			  = gibs_load_gibs_data();
	const gibs_lut_data			lut_data		  = gibs_load_gibs_lut_data();
	texture_2d<float>			depth_tex		  = global_resource_buffer[depth_buffer_texture_id];
	texture_2d<uint32_2>		gbuffer			  = global_resource_buffer[data.h_gbuffer_srv_id];
	byte_array<gibs_cell_entry> cell_entry_arr	  = gibs_load_cell_entry_arr(data);
	byte_array<uint32>			cell_to_surfel_id = gibs_load_cell_to_surfel_id_arr(data);

	array<surfel>					   surfel_arr  = gibs_load_surfel_arr(data);
	rw_byte_array<surfel_recycle_data> recycle_arr = gibs_load_surfel_recycle_data_rw_arr(data);

	const float z_depth = is_thread_valid ? load(depth_tex, dispatch_thread_id.x, dispatch_thread_id.y, 0) : 0.f;

	if (z_depth == 0.f)
	{
		is_thread_valid = false;
	}

	const uint32 object_id			   = is_thread_valid ? load(gbuffer, dispatch_thread_id.x, dispatch_thread_id.y, 0).x : invalid_id_uint32;
	const uint32 px_normal_oct_snorm16 = is_thread_valid ? load(gbuffer, dispatch_thread_id.x, dispatch_thread_id.y, 0).y : 0;
	const float3 px_normal			   = is_thread_valid ? decode_oct_snorm16(px_normal_oct_snorm16) : (float3)0;

	const float2 screen_pos = float2(dispatch_thread_id.x + 0.5f, dispatch_thread_id.y + 0.5f);

	const float2 ndc = screen_to_ndc(screen_pos, inv_backbuffer_size);

	const float4 clip_pos  = mul(view_proj_inv, float4(ndc, z_depth, 1.0));
	const float3 world_pos = clip_pos.xyz / clip_pos.w;

	const int32_4 cell_idx		= gibs_calc_cell_idx(data, lut_data, world_pos);
	const uint32  cell_idx_flat = gibs_flatten_cell_idx(data, cell_idx);

	gibs_cell_entry cell_tmp = (gibs_cell_entry)0;
	if (is_thread_valid) { cell_tmp = cell_entry_arr[cell_idx_flat]; }
	const gibs_cell_entry cell_entry = cell_tmp;

	float coverage = is_thread_valid ? 0.f : float_max;

	float  max_contribution			  = 0.f;
	uint32 max_contribution_surfel_id = invalid_id_uint32;

	// radiance for new_born, w == weight_sum
	// float4 radiance		 = (float4)0;
	float4 radiance_shared = (float4)0;

	for (uint32 i = 0; is_thread_valid and i < cell_entry.count; ++i)
	{
		const uint32			  surfel_id	   = cell_to_surfel_id[cell_entry.offset + i];
		const surfel			  surfel	   = surfel_arr[surfel_id];
		const surfel_recycle_data recycle	   = recycle_arr[surfel_id];
		const float				  contribution = gibs_calc_surfel_contribution<false>(data, surfel, world_pos, px_normal);

		if (surfel.radius == 0.f or contribution == 0.f) { continue; }

		const float fallback_contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);

		coverage += contribution;

		radiance_shared += float4(surfel.radiance, 1.f)
						 * fallback_contribution
						 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born))
						 * gibs_calc_visibility(data, surfel_id, surfel, world_pos);

		if (contribution > 0.f and max_contribution < contribution)
		{
			max_contribution		   = contribution;
			max_contribution_surfel_id = surfel_id;
		}
	}

	if (radiance_shared.w > 0.f)
	{
		rw_texture_2d<float3> gi_resolve_buffer	 = global_resource_buffer[data.h_gi_resolve_buffer_uav_id];
		radiance_shared.xyz						/= radiance_shared.w;
		// todo temporal blend
		gi_resolve_buffer[dispatch_thread_id.xy] = radiance_shared.xyz;
	}
	else
	{
		// rw_texture_2d<float3> gi_resolve_buffer	 = global_resource_buffer[data.h_gi_resolve_buffer_uav_id];
		// gi_resolve_buffer[dispatch_thread_id.xy] = color_red.xyz;
		// todo, fallback to probe
	}

	// min coverage => spawn surfel
	const uint16 linear_id		 = uint16(group_thread_id.y * GIBS_SCREEN_TILE_SIZE + group_thread_id.x);
	const uint32 coverage_packed = is_thread_valid ? as_uint(f32tof16(coverage)) << 16u | linear_id : invalid_id_uint32;

	// check spawn
	const uint32 coverage_packed_wave_min = wave_active_min(coverage_packed);

	if (wave_is_first_lane())
	{
		gs_scratch[linear_id / 32] = coverage_packed_wave_min;
	}

	group_memory_barrier_with_sync();

	const uint32 coverage_packed_group_min = wave_active_min(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
																 ? gs_scratch[wave_lane_index()]
																 : uint32_max);

	const bool need_spawn = coverage < GIBS_SPAWN_COVERAGE
						and linear_id == uint32_lower_to_uint16(coverage_packed_group_min)
						and cell_entry.count < 128
						and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0);

	// check kill
	const uint32 coverage_packed_wave_max = wave_active_max(coverage_packed);

	if (wave_is_first_lane())
	{
		gs_scratch[linear_id / 32] = coverage_packed_wave_max;
	}

	group_memory_barrier_with_sync();

	const uint32 coverage_packed_group_max = wave_active_max(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
																 ? gs_scratch[wave_lane_index()]
																 : 0u);

	const bool need_kill = coverage > GIBS_KILL_COVERAGE
					   and linear_id == uint32_lower_to_uint16(coverage_packed_group_max)
		/*or cell_entry.count > 127*/;


	assert((need_spawn and need_kill) is_false);

	const float rnd = random_pcg3d(uint32_3(dispatch_thread_id.xy, frame_index)).x;
	if (need_spawn)
	{
		const float spawn_prob = (GIBS_SPAWN_COVERAGE - coverage) / float(GIBS_SPAWN_COVERAGE)
							   * px_world_area(dispatch_thread_id.xy, world_pos - camera_pos, px_normal, camera_forward, tan_fov_y_half, backbuffer_size.y)
							   // * (1.f + dead_stack.size() / float(data.max_surfel_count))
							   * (coverage == 0.f ? 100.f : 1.f)
							   * GIBS_SPAWN_PROB_FACTOR;


		if (rnd < spawn_prob)
		{
			const object_data obj = load_object_data(object_id);

			surfel_spawn_data spawn_data = surfel_spawn_data::init_spawn(
				object_id,
				rotate_inv(obj.quaternion, world_pos - obj.pos) / obj.scale,
				encode_oct_snorm16(normalize(rotate_inv(obj.quaternion, px_normal) * obj.scale)));

			gibs_load_surfel_spawn_kill_stack(data).push(spawn_data);
		}
	}
	else if (need_kill)
	{
		const float kill_prob = (coverage - GIBS_KILL_COVERAGE) / float(GIBS_KILL_COVERAGE)
							  //* (0.01)
							  // * calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth) / (cam_far_z - cam_near_z)
							  // * (1.f + alive_stack.size() / float(data.max_surfel_count))
							  * GIBS_KILL_PROB_FACTOR;
		if (rnd < kill_prob)
		{
			gibs_load_surfel_spawn_kill_stack(data).push(surfel_spawn_data::init_kill(max_contribution_surfel_id));
		}
	}
}