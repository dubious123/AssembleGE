#include "hrp_common.asli"

void
handle_kill_surfel(const gist_data data, uint32 alive_idx, uint32 surfel_id)
{
	rw_stack<uint32> dead_stack = gist::cell::dead_id_stack(data);
	dead_stack.push(surfel_id);
	return;
};

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 group_id		   sv_group_id,
		uint32 group_thread_id sv_group_thread_id,
		uint32 alive_idx_prev  sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	byte_array<uint32> alive_id_arr_prev = gist::cell::alive_id_arr_prev(data);

	if (alive_idx_prev >= alive_id_arr_prev.size()) { return; }

	uint32 surfel_id = alive_id_arr_prev[alive_idx_prev];

	structured_buffer<gist_cell_surfel_geometry> surfel_geo_buffer = global_resource_buffer[data.h_cell_surfel_geo_buffer_srv_id];
	rw_structured_buffer<gist_cell_surfel>		 surfel_buffer	   = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];

	const gist_cell_surfel			surfel	   = surfel_buffer[surfel_id];
	const gist_cell_surfel_geometry surfel_geo = surfel_geo_buffer[surfel_id];

	if (is_object_id_valid(surfel_geo.object_id) is_false /*or object_id == invalid_id*/)
	{
		handle_kill_surfel(data, alive_idx_prev, surfel_id);
		return;
	}

	bool kill_surfel = false;

	if (surfel.recycle_data.frame_since_ref() >= 30)
	{
		kill_surfel = true;
	}

	const object_data		 obj		 = load_object_data(surfel_geo.object_id);
	const object_render_data render_data = load_object_render_data(surfel_geo.object_id);

	if (surfel_geo.primitive_id >= render_data.rt_index_buffer_size)
	{
		kill_surfel = true;
	}

	if (kill_surfel)
	{
		attr_branch()

		if (gist::debug::freeze_cell_surfel_kill(data) is_false)
		{
			handle_kill_surfel(data, alive_idx_prev, surfel_id);
			return;
		}
	}

	// alive stack prev -> alive stack curr
	rw_stack<uint32> alive_stack_curr = gist::cell::alive_id_stack_curr(data);

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
}