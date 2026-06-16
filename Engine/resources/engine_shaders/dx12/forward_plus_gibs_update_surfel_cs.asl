#include "forward_plus_common.asli"

struct fn_cell_update
{
	surfel surfel;

	static fn_cell_update
	init(const struct surfel surfel)
	{
		fn_cell_update res;
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
	}
};

void
handle_kill_surfel(const gibs_data data, uint32 alive_idx, uint32 surfel_id)
{
	const rw_stack<uint32> dead_stack = gibs_load_dead_surfel_id_stack(data);
	dead_stack.push(surfel_id);
	return;
};

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(
	uint32 group_id		   sv_group_id,
	uint32 group_thread_id sv_group_thread_id,
	uint32 alive_idx_prev  sv_dispatch_thread_id)

{
	const gibs_data		   data					  = gibs_load_gibs_data();
	const rw_stack<uint32> alive_stack_prev		  = gibs_load_alive_surfel_id_stack_prev(data);
	const uint32		   alive_count_prev_total = alive_stack_prev.size();
	if (alive_idx_prev >= alive_count_prev_total) { return; }

	uint32 surfel_id = alive_stack_prev[alive_idx_prev];

	rw_byte_array<surfel_geometry>	   geo_arr		= gibs_load_surfel_geometry_rw_arr(data);
	rw_byte_array<surfel_recycle_data> recycle_arr	= gibs_load_surfel_recycle_data_rw_arr(data);
	texture_2d<uint32_2>			   gbuffer		= global_resource_buffer[data.h_gbuffer_srv_id];
	texture_2d<float>				   depth_buffer = global_resource_buffer[depth_buffer_texture_id];
	rw_array<surfel>				   surfel_arr	= gibs_load_surfel_rw_arr(data);
	rw_byte_array<surfel_msme>		   msme_arr		= gibs_load_surfel_msme_rw_arr(data);

	surfel				  surfel		 = surfel_arr[surfel_id];
	const surfel_geometry surfel_geo	 = geo_arr[surfel_id];
	surfel_recycle_data	  surfel_recycle = recycle_arr[surfel_id];

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


	const bool in_screen = clip_pos.w > 0.f and all(ndc.xy >= -1.f) and all(ndc.xy <= 1.f) and ndc.z >= 0.f and ndc.z <= 1.f;


	const float z_depth		= in_screen
								? load(depth_buffer, screen_pos)
								: 0.f;
	const bool	surfel_seen = in_screen
						  and z_depth != 0.f
						  and ndc.z >= (z_depth - epsilon_1e4)
						  and ndc.z - z_depth < 0.5f;	 // z_depth + 0.5f - surfel - z_depth - 0.f(far)

	const uint32 g_object_id = in_screen and z_depth != 0.f
								 ? load(gbuffer, screen_pos).x
								 : invalid_id_uint32;

	bool kill_surfel = false;

	if (surfel.radius < epsilon_1e4)
	{
		kill_surfel = true;
	}
	else if (surfel_seen and surfel_geo.object_id != g_object_id)
	{
		kill_surfel = true;
	}
	else if (surfel_seen is_false and surfel_recycle.frame_since_ref() > 0xfe)
	{
		kill_surfel = true;
	}
	// else if (/*alive_count_prev_total > 0.8f * data.max_surfel_count and */ surfel_recycle.frame_since_seen() > 128u)
	//{
	//	kill_surfel = true;
	// }

	if (surfel_recycle.frame_since_born == 0u)
	{
		kill_surfel = false;
	}

	if (kill_surfel)
	{
		handle_kill_surfel(data, alive_idx_prev, surfel_id);
		return;
	}

	assert(surfel_recycle.frame_since_seen() <= 0xfff, g::fmt_gibs_update_surfels);


	surfel.normal_oct_snorm16 = encode_oct_snorm16(world_normal);
	surfel.radius			  = gibs_calc_surfel_radius(data, gibs_load_gibs_lut_data(), surfel);
	surfel.position			  = world_pos /*+ world_normal * surfel.radius * 0.1f*/;
	if (surfel_recycle.frame_since_born == 0u)
	{
		texture_2d<float3> gi_resolve_buffer = global_resource_buffer[data.h_gi_resolve_buffer_srv_id];

		const float2 uv = ndc.xy * float2(0.5, -0.5) + 0.5;
		surfel.radiance = sample_level(gi_resolve_buffer, get_linear_clamp_sampler(), uv, 0);

		surfel_msme msme = msme_arr[surfel_id];

		msme.mean_long	   = surfel.radiance;
		msme.mean_short	   = surfel.radiance;
		msme.vbbr		   = 0.f;
		msme.variance	   = float3(1.f, 1.f, 1.f);
		msme.inconsistency = 1.f;

		msme_arr.store(surfel_id, msme);
	}

	surfel_recycle.next_frame(surfel_seen);
	recycle_arr.store(surfel_id, surfel_recycle);
	// alive stack prev -> alive stack curr
	const rw_stack<uint32> alive_stack_curr = gibs_load_alive_surfel_id_stack_curr(data);

	const uint32 alive_count  = wave_active_count_bits(true);
	const uint32 local_offset = wave_prefix_count_bits(true);

	uint32 target_offset = 0u;
	if (wave_is_first_lane())
	{
		target_offset = alive_stack_curr.inc_size_atomic(alive_count);
	}

	target_offset = wave_read_lane_first(target_offset) + local_offset;

	alive_stack_curr.set(target_offset, surfel_id);

	const uint32 alive_idx_curr = target_offset;

	// surfel.alive_idx = alive_idx_curr;
	surfel_arr.store(surfel_id, surfel);

	// cell update
	fn_cell_update fn = fn_cell_update::init(surfel);
	gibs_foreach_neighbor_cell(fn, data, gibs_load_gibs_lut_data(), surfel.position);

	// calc ideal ray count

	rw_byte_array<uint32> ray_count_arr			  = gibs_load_surfel_ray_count_ideal_rw_arr(data);
	rw_byte_array<uint32> ray_count_group_sum_arr = gibs_load_surfel_ray_count_prefix_rw_arr(data);
	const surfel_msme	  msme					  = msme_arr[surfel_id];

	assert(msme.inconsistency <= 1.f);
	uint32 ray_count_ideal = uint32(lerp(GIBS_MIN_RAY_PER_SURFEL, GIBS_MAX_RAY_PER_SURFEL, saturate(msme.inconsistency)));

	if (surfel_seen is_false)
	{
		ray_count_ideal /= 4;
	}
	if (surfel_recycle.frame_since_born < 32)
	{
		ray_count_ideal = GIBS_MAX_RAY_PER_SURFEL;
	}

	ray_count_arr.store(alive_idx_curr, ray_count_ideal);

	uint32 ray_count_ideal_wave_sum = wave_active_sum(ray_count_ideal);

	if (wave_is_first_lane())
	{
		ray_count_group_sum_arr.store(group_id, ray_count_ideal_wave_sum);
	}
}