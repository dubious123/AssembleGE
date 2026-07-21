#include "hrp_common.asli"

void
handle_kill_probe(const gibs_data data, uint32 alive_idx, uint32 probe_id)
{
	rw_stack<uint32> dead_stack = gibs::probe::dead_id_stack(data);
	dead_stack.push(probe_id);
	return;
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id,
		uint32 alive_idx_prev  sv_dispatch_thread_id)

{
	const gibs_data data = gibs::load_data();

	rw_stack<uint32> alive_stack_prev		= gibs::probe::alive_id_stack_prev(data);
	const uint32	 alive_count_prev_total = alive_stack_prev.size();

	if (alive_idx_prev >= alive_count_prev_total) { return; }

	uint32 probe_id = alive_stack_prev[alive_idx_prev];

	structured_buffer<gibs_surfel_probe_geometry> probe_geo_buffer	   = global_resource_buffer[data.h_surfel_probe_geo_buffer_srv_id];
	rw_structured_buffer<gibs_recycle_data>		  probe_recycle_buffer = global_resource_buffer[data.h_surfel_probe_recycle_buffer_uav_id];

	const gibs_recycle_data			 probe_recycle = probe_recycle_buffer[probe_id];
	const gibs_surfel_probe_geometry probe_geo	   = probe_geo_buffer[probe_id];

	if (is_object_id_valid(probe_geo.object_id) is_false /*or object_id == invalid_id*/)
	{
		handle_kill_probe(data, alive_idx_prev, probe_id);
		return;
	}

	bool kill_probe = false;

	if (probe_recycle.frame_since_ref() >= 0xff)
	{
		kill_probe = true;
	}

	const object_data		 obj		 = load_object_data(probe_geo.object_id);
	const object_render_data render_data = load_object_render_data(probe_geo.object_id);

	if (probe_geo.primitive_id >= render_data.rt_index_buffer_size)
	{
		kill_probe = true;
	}

	if (kill_probe and (gibs::debug::freeze_spawn_kill(data) is_false))
	{
		handle_kill_probe(data, alive_idx_prev, probe_id);
		return;
	}

	// alive stack prev -> alive stack curr
	rw_stack<uint32> alive_stack_curr = gibs::probe::alive_id_stack_curr(data);

	const uint32 alive_count		= wave_active_count_bits(true);
	const uint32 alive_local_offset = wave_prefix_count_bits(true);

	uint32 alive_offset = 0u;
	if (wave_is_first_lane())
	{
		alive_offset = alive_stack_curr.inc_size_atomic(alive_count);

		assert(alive_offset + alive_count <= alive_stack_curr.capacity, line);
	}

	alive_offset = wave_read_lane_first(alive_offset) + alive_local_offset;

	alive_stack_curr.set(alive_offset, probe_id);
}