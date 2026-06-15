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

	const gibs_data					   data				 = gibs_load_gibs_data();
	texture_2d<float>				   depth_tex		 = global_resource_buffer[depth_buffer_texture_id];
	texture_2d<uint32_2>			   gbuffer			 = global_resource_buffer[data.h_gbuffer_srv_id];
	texture_2d<float2>				   visibility_atlas	 = global_resource_buffer[data.h_visibility_atlas_srv_id];
	byte_array<gibs_cell_entry>		   cell_entry_arr	 = gibs_load_cell_entry_arr(data);
	byte_array<uint32>				   cell_to_surfel_id = gibs_load_cell_to_surfel_id_arr(data);
	rw_array<surfel>				   surfel_arr		 = gibs_load_surfel_rw_arr(data);
	rw_byte_array<surfel_geometry>	   geo_arr			 = gibs_load_surfel_geometry_rw_arr(data);
	rw_byte_array<surfel_recycle_data> recycle_arr		 = gibs_load_surfel_recycle_data_rw_arr(data);
	rw_byte_array<surfel_msme>		   msme_arr			 = gibs_load_surfel_msme_rw_arr(data);

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

	const int32_4 cell_idx		= gibs_calc_cell_idx(data, gibs_load_gibs_lut_data(), world_pos);
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
		const uint32			  surfel_id = cell_to_surfel_id[cell_entry.offset + i];
		const surfel			  surfel	= surfel_arr[surfel_id];
		const surfel_recycle_data recycle	= recycle_arr[surfel_id];

		if (surfel.radius == 0.f) { continue; }

		const float contribution = gibs_calc_surfel_contribution<false>(data, surfel, world_pos, px_normal);
		// const float fallback_contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);

		coverage += contribution;

		// const float visibility = gibs_calc_visibility(data, surfel_id, surfel, world_pos);

		// if (visibility == 0.f) { continue; }

		// radiance_shared += float4(surfel.radiance, 1.f)
		//				 * fallback_contribution
		//				 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born))
		//				 * visibility;

		if (contribution > 0.f and max_contribution < contribution)
		{
			max_contribution		   = contribution;
			max_contribution_surfel_id = surfel_id;
		}
	}

	// min coverage => spawn surfel
	const uint16 linear_id		 = uint16(group_thread_id.y * GIBS_SCREEN_TILE_SIZE + group_thread_id.x);
	const uint32 coverage_packed = is_thread_valid ? as_uint(f32tof16(coverage)) << 16u | linear_id : invalid_id_uint32;

	const float rnd = random_pcg3d(uint32_3(dispatch_thread_id.xy, frame_index)).x;

	attr_branch()

	if ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) is_false)
	{
		const uint32 coverage_packed_wave_min = wave_active_min(coverage_packed);

		if (wave_is_first_lane())
		{
			gs_scratch[linear_id / 32] = coverage_packed_wave_min;
		}

		group_memory_barrier_with_sync();

		const uint32 coverage_packed_group_min = wave_active_min(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
																	 ? gs_scratch[wave_lane_index()]
																	 : uint32_max);

		if (coverage < GIBS_SPAWN_COVERAGE and linear_id == uint32_lower_to_uint16(coverage_packed_group_min))
		{
			assert(is_thread_valid, g::fmt_gibs_tile_coverage);

			rw_stack<uint32> dead_stack	 = gibs_load_dead_surfel_id_stack(data);
			rw_stack<uint32> alive_stack = gibs_load_alive_surfel_id_stack_curr(data);

			const float spawn_prob = (GIBS_SPAWN_COVERAGE - coverage) / float(GIBS_SPAWN_COVERAGE)
								   * px_world_area(dispatch_thread_id.xy, world_pos - camera_pos, px_normal, camera_forward, tan_fov_y_half, backbuffer_size.y)
								   // * (1.f + dead_stack.size() / float(data.max_surfel_count))
								   * (coverage == 0.f ? 100.f : 1.f)
								   * GIBS_SPAWN_PROB_FACTOR;
			// spawn surfel
			uint32 new_surfel_id;
			if (rnd < spawn_prob and dead_stack.try_pop(new_surfel_id))
			{
				for (uint32 i = 0; i < cell_entry.count; ++i)
				{
					const uint32			  surfel_id = cell_to_surfel_id[cell_entry.offset + i];
					const surfel			  surfel	= surfel_arr[surfel_id];
					const surfel_recycle_data recycle	= recycle_arr[surfel_id];

					if (surfel.radius == 0.f) { continue; }

					const float fallback_contribution = gibs_calc_surfel_contribution<true>(data, surfel, world_pos, px_normal);

					const float visibility = gibs_calc_visibility(data, surfel_id, surfel, world_pos);

					if (visibility == 0.f) { continue; }

					radiance_shared += float4(surfel.radiance, 1.f)
									 * fallback_contribution
									 * smoothstep(0.f, float(GIBS_RADIANCE_CACHE_DELAY), float(recycle.frame_since_born))
									 * visibility;
				}


				const uint32 alive_idx = alive_stack.push(new_surfel_id);

				surfel surfel = surfel_arr[new_surfel_id];
				// surfel.alive_idx = alive_idx;

				surfel.position			  = world_pos;
				surfel.normal_oct_snorm16 = px_normal_oct_snorm16;
				surfel.radius			  = gibs_calc_surfel_radius(data, gibs_load_gibs_lut_data(), surfel);

				// todo, radiance is too dark
				// todo, radiance is now too bright
				surfel.radiance = radiance_shared.w > 0.f ? radiance_shared.xyz / radiance_shared.w : (float3)0;
				// surfel.radiance = shared_count > 0 ? radiance_shared.xyz / shared_count : (float3)0;
				// surfel.radiance = radiance.w > 0.f ? radiance.xyz / radiance.w : (float3)0;

				surfel_arr.store(new_surfel_id, surfel);

				surfel_geometry	  geo		 = geo_arr[new_surfel_id];
				const object_data obj		 = load_object_data(object_id);
				geo.object_id				 = object_id;
				geo.local_pos				 = rotate_inv(obj.quaternion, world_pos - obj.pos) / obj.scale;
				geo.local_normal_oct_snorm16 = encode_oct_snorm16(normalize(rotate_inv(obj.quaternion, px_normal) * obj.scale));
				geo_arr.store(new_surfel_id, geo);


				surfel_recycle_data recycle		   = recycle_arr[new_surfel_id];
				recycle.frame_since_born		   = 0u;
				recycle.frame_since_seen_and_extra = 0u;
				recycle_arr.store(new_surfel_id, recycle);

				surfel_msme msme = msme_arr[new_surfel_id];

				msme.mean_long	   = surfel.radiance;
				msme.mean_short	   = surfel.radiance;
				msme.vbbr		   = 0.f;
				msme.variance	   = float3(1.f, 1.f, 1.f);
				msme.inconsistency = 1.f;

				msme_arr.store(new_surfel_id, msme);
			}
		}
	}
	// max coverage => kill surfel
	{
		const uint32 coverage_packed_wave_max = wave_active_max(coverage_packed);

		if (wave_is_first_lane())
		{
			gs_scratch[linear_id / 32] = coverage_packed_wave_max;
		}

		group_memory_barrier_with_sync();

		const uint32 coverage_packed_group_max = wave_active_max(wave_lane_index() < GIBS_SCREEN_GROUP_SHARED_SIZE
																	 ? gs_scratch[wave_lane_index()]
																	 : 0u);

		if (coverage > GIBS_KILL_COVERAGE and linear_id == uint32_lower_to_uint16(coverage_packed_group_max))
		{
			const float kill_prob = (coverage - GIBS_KILL_COVERAGE) / float(GIBS_KILL_COVERAGE)
								  //* (0.01)
								  // * calc_linear_z_reversed(cam_near_z, cam_far_z, z_depth) / (cam_far_z - cam_near_z)
								  // * (1.f + alive_stack.size() / float(data.max_surfel_count))
								  * GIBS_KILL_PROB_FACTOR;

			if (rnd < kill_prob)
			{
				assert(max_contribution_surfel_id != invalid_id_uint32, g::fmt_gibs_tile_coverage);
				surfel surfel = surfel_arr[max_contribution_surfel_id];

				surfel.kill();
				surfel_arr.store(max_contribution_surfel_id, surfel);
			}
		}
	}
}