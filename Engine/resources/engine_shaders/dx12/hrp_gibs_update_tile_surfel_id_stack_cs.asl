#include "hrp_common.asli"

void
handle_kill_surfel(const gibs_data data, uint32 alive_idx, uint32 surfel_id)
{
	rw_stack<uint32> dead_stack = gibs::tile::dead_id_stack(data);
	dead_stack.push(surfel_id);
	return;
};

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id,
		uint32 alive_idx_prev  sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	rw_stack<uint32> alive_stack_prev		= gibs::tile::alive_id_stack_prev(data);
	const uint32	 alive_count_prev_total = alive_stack_prev.size();
	if (alive_idx_prev >= alive_count_prev_total) { return; }

	uint32 surfel_id = alive_stack_prev[alive_idx_prev];

	texture_2d<uint32_2>						 gbuffer	  = global_resource_buffer[opaque_gbuffer_srv_id];
	texture_2d<float>							 depth_buffer = global_resource_buffer[opaque_depth_buffer_srv_id];
	rw_structured_buffer<gibs_tile_surfel>		 surfel_arr	  = global_resource_buffer[data.h_tile_surfel_buffer_uav_id];
	structured_buffer<gibs_tile_surfel_geometry> geo_arr	  = global_resource_buffer[data.h_tile_surfel_geo_buffer_srv_id];

	gibs_tile_surfel				surfel	   = surfel_arr[surfel_id];
	const gibs_tile_surfel_geometry surfel_geo = geo_arr[surfel_id];

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

	const bool surfel_seen = in_screen
						 and dot(px_normal, world_normal) > 0.9f
						 and z_depth != 0.f
						 and ndc.z >= (z_depth - epsilon_1e4)
						 and ndc.z < (z_depth + epsilon_1e4);	 // z_depth + 0.5f - surfel - z_depth - 0.f(far)

	const uint32 obj_id = in_screen and z_depth != 0.f
							? load_opaque_meshlet_render_data(gbuffer[screen_pos].x & 0x01ffffff).object_id
							: invalid_id_uint32;


	bool kill_surfel = false;

	if (surfel.radius < epsilon_1e4)
	{
		kill_surfel = true;
	}
	else if (surfel_seen and surfel_geo.object_id != obj_id)
	{
		kill_surfel = true;
	}
	else if (/*surfel_seen is_false and*/ surfel.recycle_data.frame_since_ref() >= 0xff)
	{
		kill_surfel = true;
	}

	if (surfel.recycle_data.frame_since_born() == 0u)
	{
		kill_surfel = false;
	}

	if (kill_surfel and (gibs::debug::freeze_spawn_kill(data) is_false))
	{
		handle_kill_surfel(data, alive_idx_prev, surfel_id);
		return;
	}

	surfel.normal_oct_snorm16 = encode_oct_snorm16(world_normal);
	surfel.position			  = world_pos + world_normal * surfel.radius * 0.001f;

	attr_branch()

	if (gibs::debug::freeze_spawn_kill(data) is_false)
	{
		surfel.radius = gibs::tile::calc_surfel_radius(data, gibs::load_lut_data(), surfel.position);

		if (surfel_seen)
		{
			surfel.recycle_data.set_ref();
		}

		surfel.recycle_data.next_frame();
	}

	// alive stack prev -> alive stack curr
	rw_stack<uint32> alive_stack_curr = gibs::tile::alive_id_stack_curr(data);

	const bool is_tile = surfel_seen;
	const bool is_cell = surfel_seen is_false;

	const uint32 alive_count		= wave_active_count_bits(true);
	const uint32 alive_local_offset = wave_prefix_count_bits(true);

	uint32 alive_offset = 0u;
	if (wave_is_first_lane())
	{
		alive_offset = alive_stack_curr.inc_size_atomic(alive_count);

		assert(alive_offset + alive_count <= alive_stack_curr.capacity, line);
	}

	alive_offset = wave_read_lane_first(alive_offset) + alive_local_offset;

	alive_stack_curr.set(alive_offset, surfel_id);

	surfel_arr[surfel_id] = surfel;
}