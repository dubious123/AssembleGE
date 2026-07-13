#include "hrp_common.asli"

struct fn_fill_tile_to_surfel
{
	uint32 surfel_id;

	uint32_4 screen_aabb;

	static fn_fill_tile_to_surfel
	init(uint32 surfel_id, const struct surfel surfel, const float3 world_normal)
	{
		fn_fill_tile_to_surfel res;

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

		res.surfel_id = surfel_id;

		return res;
	}

	void
	operator()(const gibs_data data, uint32_2 tile_idx)
	{
		uint32_2 tile_min = tile_idx * GIBS_GI_RESOLVE_TILE_SIZE;
		uint32_2 tile_max = tile_min + GIBS_GI_RESOLVE_TILE_SIZE - 1;

		if (aabb_intersect(screen_aabb, uint32_4(tile_min, tile_max)) is_false) { return; }

		rw_byte_array<gibs_tile_entry> entry_arr = gibs_load_tile_entry_rw_arr(data);

		const uint32 tile_idx_flat = gibs_flatten_tile_idx(data, tile_idx);

		const gibs_tile_entry entry = entry_arr[tile_idx_flat];										  // don't read count

		const uint32		  local_offset			= entry_arr.atomic_inc(tile_idx_flat, 1u, 1u);	  // entry.count++;
		rw_byte_array<uint32> tile_to_surfel_id_arr = gibs_load_tile_to_surfel_id_rw_arr(data);

		assert(tile_idx_flat < entry_arr.capacity, g::fmt_forward_plus_gibs_cell_to_surfel_scatter_cs, line, tile_idx);

		tile_to_surfel_id_arr.store(entry.offset + local_offset, surfel_id);
	}
};

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 alive_idx sv_dispatch_thread_id)

{
	const gibs_data			 data			= gibs_load_gibs_data();
	const byte_array<uint32> alive_arr_curr = gibs_load_alive_arr_curr(data);

	if (alive_idx >= alive_arr_curr.capacity) { return; }

	const uint32 surfel_id = alive_arr_curr[alive_idx];

	const surfel surfel = gibs_load_surfel_arr(data)[surfel_id];

	if (surfel.surfel_seen())
	{
		const float4 clip_pos = mul(view_proj, float4(surfel.position, 1.f));
		const float3 ndc	  = clip_pos.xyz / clip_pos.w;

		const float3		   world_normal = decode_oct_snorm16(surfel.normal_oct_snorm16);
		fn_fill_tile_to_surfel fn			= fn_fill_tile_to_surfel::init(surfel_id, surfel, world_normal);

		gibs_foreach_neighbor_tile(fn, data, min(int32_2(ndc_xy_to_screen(ndc.xy, backbuffer_size)), int32_2(backbuffer_size) - 1));
	}
	else
	{
	}
}