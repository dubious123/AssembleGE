#include "hrp_common.asli"

struct fn_tile_alloc
{
	uint32_4 screen_aabb;

	static fn_tile_alloc
	init(const gibs_tile_surfel surfel)
	{
		fn_tile_alloc res;

		const float3 world_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);

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

		const uint32 tile_id = gibs::tile::calc_id(data, tile_idx);
		const bool	 succeed = gibs::tile::add_surfel(data, tile_id);

		assert(tile_id < data.tile_count_total, line);
		assert(succeed, line);
	}
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id,
		uint32 alive_id		   sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	byte_array<uint32> alive_stack_curr = gibs::tile::alive_id_arr_curr(data);
	if (alive_id >= alive_stack_curr.size()) { return; }

	const uint32 surfel_id = alive_stack_curr[alive_id];

	structured_buffer<gibs_tile_surfel>	   surfel_arr = global_resource_buffer[data.h_tile_surfel_buffer_srv_id];
	rw_structured_buffer<gibs_surfel_msme> msme_arr	  = global_resource_buffer[data.h_tile_surfel_msme_buffer_uav_id];

	const gibs_tile_surfel surfel = surfel_arr[surfel_id];
	const gibs_surfel_msme msme	  = msme_arr[surfel_id];

	fn_tile_alloc fn = fn_tile_alloc::init(surfel);
	gibs::foreach_neighbor_tile(fn, data, min(int32_2(ndc_xy_to_screen(world_to_ndc(view_proj, surfel.position).xy, backbuffer_size)), int32_2(backbuffer_size) - 1));

	// uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, msme.inconsistency * 4));
	// uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, saturate(sqrt(max(0.f, msme.incon_var)) * 10)));

	uint16 ideal_ray_count = uint16(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, msme.incon_var * (1.f / GIBS_MSME_INCON_BLEND)));

	if (surfel.recycle_data.frame_since_born() < GIBS_RADIANCE_CACHE_DELAY)
	{
		ideal_ray_count = GIBS_MAX_RAY_PER_SURFEL;
	}

	gibs::tile::set_ideal_ray_count(data, alive_id, ideal_ray_count);

	const uint16 ideal_ray_count_wave_sum = wave_active_sum(ideal_ray_count);

	if (wave_is_first_lane())
	{
		gibs::tile::set_ideal_ray_count_wave_sum(data, group_id, ideal_ray_count_wave_sum);
	}
}