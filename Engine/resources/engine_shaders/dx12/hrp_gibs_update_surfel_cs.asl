#include "hrp_common.asli"

struct fn_tile_alloc
{
	uint32_4 screen_aabb;

	static fn_tile_alloc
	init(const struct surfel surfel, const float3 world_normal)
	{
		fn_tile_alloc res;

		const float3x3 transform_t = gen_world_normal_transform_t(world_normal);
		const float3   surfel_rf   = surfel.position + mul(float3(surfel.radius, 0, surfel.radius), transform_t);
		const float3   surfel_lf   = surfel.position + mul(float3(-surfel.radius, 0, surfel.radius), transform_t);
		const float3   surfel_rb   = surfel.position + mul(float3(surfel.radius, 0, -surfel.radius), transform_t);
		const float3   surfel_lb   = surfel.position + mul(float3(-surfel.radius, 0, -surfel.radius), transform_t);

		const float4 clip_pos_rf = mul(view_proj, float4(surfel_rf, 1.f));
		const float3 ndc_rf		 = clamp(clip_pos_rf.xyz / clip_pos_rf.w, -1.f, 1.f);

		const int32_2 screen_pos_rf = int32_2(ndc_xy_to_screen(ndc_rf.xy, backbuffer_size));

		const float4 clip_pos_lf = mul(view_proj, float4(surfel_lf, 1.f));
		const float3 ndc_lf		 = clamp(clip_pos_lf.xyz / clip_pos_lf.w, -1.f, 1.f);

		const int32_2 screen_pos_lf = int32_2(ndc_xy_to_screen(ndc_lf.xy, backbuffer_size));

		const float4 clip_pos_rb = mul(view_proj, float4(surfel_rb, 1.f));
		const float3 ndc_rb		 = clamp(clip_pos_rb.xyz / clip_pos_rb.w, -1.f, 1.f);

		const int32_2 screen_pos_rb = int32_2(ndc_xy_to_screen(ndc_rb.xy, backbuffer_size));

		const float4 clip_pos_lb = mul(view_proj, float4(surfel_lb, 1.f));
		const float3 ndc_lb		 = clamp(clip_pos_lb.xyz / clip_pos_lb.w, -1.f, 1.f);

		const int32_2 screen_pos_lb = int32_2(ndc_xy_to_screen(ndc_lb.xy, backbuffer_size));

		if (clip_pos_rf.w <= epsilon_1e4 or clip_pos_lf.w <= epsilon_1e4 or clip_pos_rb.w <= epsilon_1e4 or clip_pos_lb.w <= epsilon_1e4)
		{
			res.screen_aabb = uint32_4(0, 0, uint32_2(backbuffer_size));
		}
		else
		{
			res.screen_aabb = uint32_4(min(screen_pos_rf, screen_pos_lf, screen_pos_rb, screen_pos_lb),
									   max(screen_pos_rf, screen_pos_lf, screen_pos_rb, screen_pos_lb));
		}

		return res;
	}

	void
	operator()(const gibs_data data, uint32_2 tile_idx)
	{
		uint32_2 tile_min = tile_idx * GIBS_GI_RESOLVE_TILE_SIZE;
		uint32_2 tile_max = tile_min + GIBS_GI_RESOLVE_TILE_SIZE - 1;

		if (aabb_intersect(screen_aabb, uint32_4(tile_min, tile_max)) is_false) { return; }

		rw_byte_array<gibs_tile_entry> arr = gibs_load_tile_entry_rw_arr(data);

		const uint32 tile_idx_flat = gibs_flatten_tile_idx(data, tile_idx);

		const uint32 count_original = arr.atomic_inc(tile_idx_flat, 1u);

		assert(tile_idx_flat < arr.capacity, g::fmt_forward_plus_gibs_update_surfel_cs, line);

		// assert(count_original < 0xffff, g::fmt_forward_plus_gibs_update_surfel_cs, line);
	}
};

struct fn_cell_alloc
{
	surfel surfel;

	static fn_cell_alloc
	init(const struct surfel surfel)
	{
		fn_cell_alloc res;
		res.surfel = surfel;
		return res;
	}

	void
	operator()(const gibs_data data, const gibs_lut_data lut_data, int32_4 cell_idx)
	{
		if (gibs_surfel_cell_intersect(data, lut_data, surfel, cell_idx) is_false) { return; }

		const uint32				   cell_idx_flat = gibs_flatten_cell_idx(data, cell_idx);
		rw_byte_array<gibs_cell_entry> arr			 = gibs_load_cell_entry_rw_arr(data);

		const uint32 count_original = arr.atomic_inc(cell_idx_flat, 1u);

		assert(cell_idx_flat < arr.capacity, g::fmt_forward_plus_gibs_update_surfel_cs, line);

		// assert(count_original < 0xffff, g::fmt_forward_plus_gibs_update_surfel_cs, line);
	}
};

void
handle_kill_surfel(const gibs_data data, uint32 alive_idx, uint32 surfel_id)
{
	rw_stack<uint32> dead_stack = gibs_load_dead_surfel_id_stack(data);
	dead_stack.push(surfel_id);
	return;
};

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id,
		uint32 alive_idx_prev  sv_dispatch_thread_id)

{
	const gibs_data data = gibs_load_gibs_data();

	rw_stack<uint32> alive_stack_prev		= gibs_load_alive_surfel_id_stack_prev(data);
	const uint32	 alive_count_prev_total = alive_stack_prev.size();
	if (alive_idx_prev >= alive_count_prev_total) { return; }

	uint32 surfel_id = alive_stack_prev[alive_idx_prev];

	texture_2d<uint32_2>		   gbuffer		= global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float>			   depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	rw_byte_array<surfel>		   surfel_arr	= gibs_load_surfel_rw_arr(data);
	rw_byte_array<surfel_geometry> geo_arr		= gibs_load_surfel_geometry_rw_arr(data);
	rw_byte_array<surfel_msme>	   msme_arr		= gibs_load_surfel_msme_rw_arr(data);

	surfel				  surfel	 = surfel_arr[surfel_id];
	const surfel_geometry surfel_geo = geo_arr[surfel_id];

	// assert(surfel.alive_idx == alive_idx_prev, g::fmt_gibs_update_surfels);

	const float3 local_normal = decode_oct_snorm16(surfel_geo.local_normal_oct_snorm16);

	if (is_object_id_valid(surfel_geo.object_id) is_false)
	{
		handle_kill_surfel(data, alive_idx_prev, surfel_id);
		return;
	}

	const object_data obj = load_object_data(surfel_geo.object_id);

	const float3 world_pos	  = rotate(obj.quaternion, surfel_geo.local_pos * obj.scale) + obj.pos;
	const float3 world_normal = normalize(rotate(obj.quaternion, local_normal / obj.scale));
	const float4 clip_pos	  = mul(view_proj, float4(world_pos, 1.f));
	const float3 ndc		  = clip_pos.xyz / clip_pos.w;

	const uint32_2 screen_pos = uint32_2(ndc_xy_to_screen(ndc.xy, backbuffer_size));


	const bool in_screen = clip_pos.w > 0.f
					   and all(ndc.xy >= -1.f)
					   and all(ndc.xy <= 1.f)
					   and ndc.z >= 0.f
					   and ndc.z <= 1.f;

	const float z_depth = in_screen
							? load(depth_buffer, screen_pos)
							: 0.f;

	const float3 px_normal = decode_oct_snorm16(load(gbuffer, screen_pos).y);

	const bool was_seen = surfel.surfel_seen();

	// const bool surfel_seen = in_screen;

	const bool surfel_seen = in_screen
						 and was_seen
						 and dot(px_normal, world_normal) > 0.9f
						 and z_depth != 0.f
						 and ndc.z >= (z_depth - epsilon_1e4)
						 and ndc.z < (z_depth + epsilon_1e4);	 // z_depth + 0.5f - surfel - z_depth - 0.f(far)

	const uint32 obj_id = in_screen and z_depth != 0.f
							? load_opaque_meshlet_render_data(gbuffer[screen_pos].x & 0x01ffffff).object_id
							: invalid_id_uint32;


	bool kill_surfel = false;

	const float2 rnd = random_pcg2d(uint32_2(surfel_id, frame_index)).x;

	if (surfel.radius < epsilon_1e4)
	{
		kill_surfel = true;
	}
	else if (surfel_seen and surfel_geo.object_id != obj_id)
	{
		kill_surfel = true;
	}
	else if (surfel_seen is_false and surfel.frame_since_ref() >= 0x7e)
	{
		kill_surfel = true;
	}
	else if (was_seen and (surfel_seen is_false) and (rnd.x < 0.9))
	{
		kill_surfel = true;
	}
	// else if (was_seen is_false and (surfel_seen))
	//{
	//	kill_surfel = true;
	// }

	if (surfel.frame_since_born() == 0u)
	{
		kill_surfel = false;
	}

	if (kill_surfel and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0))
	{
		handle_kill_surfel(data, alive_idx_prev, surfel_id);
		return;
	}

	surfel.normal_oct_snorm16 = encode_oct_snorm16(world_normal);
	surfel.position			  = world_pos + world_normal * surfel.radius * 0.001f;

	const surfel_msme msme = msme_arr[surfel_id];

	attr_branch()

	if ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0)
	{
		if (surfel_seen)
		{
			surfel.radius = gibs_calc_tile_surfel_radius(data, gibs_load_gibs_lut_data(), surfel);
		}
		else
		{
			surfel.radius = gibs_calc_cell_surfel_radius(data, gibs_load_gibs_lut_data(), surfel);
		}

		surfel.next_frame(surfel_seen);
	}


	// alive stack prev -> alive stack curr
	rw_stack<uint32> alive_stack_curr = gibs_load_alive_surfel_id_stack_curr(data);

	const bool is_tile = surfel_seen;
	const bool is_cell = surfel_seen is_false;

	// todo, optimize this using wave active ballet
	const uint32 alive_count		= wave_active_count_bits(true);
	const uint32 alive_local_offset = wave_prefix_count_bits(true);

	uint32 alive_offset = 0u;
	if (wave_is_first_lane())
	{
		alive_offset = alive_stack_curr.inc_size_atomic(alive_count);

		assert(alive_offset + alive_count <= alive_stack_curr.capacity, g::fmt_gibs_update_surfels, line);
	}

	alive_offset = wave_read_lane_first(alive_offset) + alive_local_offset;

	alive_stack_curr.set(alive_offset, surfel_id);

	const uint32 alive_idx_curr = alive_offset;

	// surfel.alive_idx = alive_idx_curr;
	surfel_arr.store(surfel_id, surfel);

	if (surfel.surfel_seen())
	{
		fn_tile_alloc fn = fn_tile_alloc::init(surfel, world_normal);
		gibs_foreach_neighbor_tile(fn, data, min(int32_2(ndc_xy_to_screen(ndc.xy, backbuffer_size)), int32_2(backbuffer_size) - 1));
	}
	else
	{
		fn_cell_alloc fn = fn_cell_alloc::init(surfel);
		gibs_foreach_neighbor_cell(fn, data, gibs_load_gibs_lut_data(), surfel.position);
	}


	// uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, msme.inconsistency * 4));
	// uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, saturate(sqrt(max(0.f, msme.incon_var)) * 10)));

	uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, msme.incon_var * (1.f / GIBS_MSME_INCON_BLEND)));

	if (surfel_seen is_false)
	{
		ray_count_ideal /= 4;
	}
	if (surfel.frame_since_born() < GIBS_RADIANCE_CACHE_DELAY)
	{
		ray_count_ideal = GIBS_MAX_RAY_PER_SURFEL;
	}

	gibs_load_surfel_ray_count_ideal_rw_arr(data).store(alive_idx_curr, ray_count_ideal);

	uint32 ray_count_ideal_wave_sum = wave_active_sum(ray_count_ideal);

	if (wave_is_first_lane())
	{
		gibs_load_surfel_ray_count_ideal_wave_sum_rw_arr(data).store(group_id, ray_count_ideal_wave_sum);
	}
}