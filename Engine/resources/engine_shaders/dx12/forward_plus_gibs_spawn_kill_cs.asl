#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data							 data			   = gibs_load_gibs_data();
	const rw_stack<uint32>					 dead_stack		   = gibs_load_dead_surfel_id_stack(data);
	const rw_stack<uint32>					 alive_stack	   = gibs_load_alive_surfel_id_stack_curr(data);
	const rw_byte_array<surfel_geometry>	 geo_arr		   = gibs_load_surfel_geometry_rw_arr(data);
	const rw_byte_array<surfel_recycle_data> recycle_data_arr  = gibs_load_surfel_recycle_data_rw_arr(data);
	const rw_byte_array<surfel_msme>		 msme_arr		   = gibs_load_surfel_msme_rw_arr(data);
	const texture_2d<float3>				 gi_resolve_buffer = global_resource_buffer[data.h_gi_resolve_buffer_srv_id];

	const rw_array<surfel>		  surfel_arr = gibs_load_surfel_rw_arr(data);
	byte_array<surfel_spawn_data> spawn_arr	 = gibs_load_surfel_spawn_kill_array(data);

	if (thread_id >= gibs_load_surfel_spawn_kill_array_size(data))
	{
		return;
	}

	const surfel_spawn_data spawn_data = spawn_arr[thread_id];

	if (spawn_data.is_spawn())
	{
		uint32 surfel_id;
		if (dead_stack.try_pop(surfel_id) is_false) { return; }

		alive_stack.push(surfel_id);
		// radiance and normal and radius will be calculate during surfel_update_cs

		surfel_geometry geo			 = geo_arr[surfel_id];
		geo.object_id				 = spawn_data.object_id_or_surfel_id;
		geo.local_pos				 = spawn_data.local_pos;
		geo.local_normal_oct_snorm16 = spawn_data.local_normal_oct_snorm16;
		geo_arr.store(surfel_id, geo);

		surfel_recycle_data recycle		   = recycle_data_arr[surfel_id];
		recycle.frame_since_born		   = 0u;
		recycle.frame_since_seen_and_extra = 0u;
		recycle_data_arr.store(surfel_id, recycle);
	}
	else
	{
		const uint32 surfel_id = spawn_data.object_id_or_surfel_id;

		surfel surfel = surfel_arr[surfel_id];

		surfel.kill();
		surfel_arr.store(surfel_id, surfel);

		surfel_recycle_data recycle		   = recycle_data_arr[surfel_id];
		recycle.frame_since_born		   = 0xffu;
		recycle.frame_since_seen_and_extra = 0xffu;
		recycle_data_arr.store(surfel_id, recycle);
	}
}