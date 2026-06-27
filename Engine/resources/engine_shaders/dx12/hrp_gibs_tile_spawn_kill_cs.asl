#include "hrp_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data						 data		 = gibs_load_gibs_data();
	const rw_byte_array<surfel>			 surfel_arr	 = gibs_load_surfel_rw_arr(data);
	const rw_byte_array<surfel_msme>	 msme_arr	 = gibs_load_surfel_msme_rw_arr(data);
	const rw_byte_array<surfel_geometry> geo_arr	 = gibs_load_surfel_geometry_rw_arr(data);
	const rw_stack<uint32>				 dead_stack	 = gibs_load_dead_surfel_id_stack(data);
	const rw_stack<uint32>				 alive_stack = gibs_load_alive_surfel_id_stack_prev(data);

	const uint32 tile_idx_flat = thread_id;

	if (tile_idx_flat >= data.tile_count_total)
	{
		return;
	}

	const gibs_tile_surfel_spawn_kill_data spawn_kill_data = gibs_load_tile_surfel_spawn_kill_data(data, tile_idx_flat);

	if (spawn_kill_data.has_kill())
	{
		const uint32 surfel_id = spawn_kill_data.surfel_id;

		surfel surfel = surfel_arr[surfel_id];

		surfel.kill();

		surfel_arr.store(surfel_id, surfel);
	}

	// todo, maybe add some radiance sharing
	if (spawn_kill_data.has_spawn() and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0))
	{
		uint32 surfel_id;
		if (dead_stack.try_pop(surfel_id) is_false) { return; }

		alive_stack.push(surfel_id);
		// radiance and normal and radius will be calculate during surfel_update_cs

		surfel surfel = surfel_arr[surfel_id];

		surfel.radiance_r11g11b10 = spawn_kill_data.radiance_r11g11b10;
		surfel.recycle_data		  = 0x8000;

		assert(surfel.is_new_born(), g::fmt_gibs_spawn_kill, line);

		surfel_arr.store(surfel_id, surfel);

		surfel_geometry geo			 = geo_arr[surfel_id];
		geo.object_id				 = spawn_kill_data.object_id;
		geo.local_pos				 = spawn_kill_data.local_pos;
		geo.local_normal_oct_snorm16 = spawn_kill_data.local_normal_oct_snorm16;
		geo_arr.store(surfel_id, geo);


		surfel_msme msme   = msme_arr[surfel_id];
		msme.mean_long	   = decode_r11g11b10(spawn_kill_data.radiance_r11g11b10);
		msme.mean_short	   = msme.mean_long;
		msme.vbbr		   = 0.f;
		msme.variance	   = float3(1.f, 1.f, 1.f);
		msme.inconsistency = 1.f;
		msme.incon_mean	   = 0.f;
		msme.incon_var	   = 0.f;

		msme_arr.store(surfel_id, msme);

		rw_byte_array<uint16> vis_arr	  = gibs_load_surfel_visibility_rw_arr(data, surfel_id);
		rw_byte_array<half>	  lum_arr	  = gibs_load_lum_rw_arr(data, surfel_id);
		rw_byte_array<half>	  lum_cdf_arr = gibs_load_lum_cdf_rw_arr(data, surfel_id);
		for (uint32 i = 0; i < GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE; ++i)
		{
			vis_arr.store(i, 1.h);
			lum_arr.store(i, 0.h);
			lum_cdf_arr.store(i, (1.h / (GIBS_ATLAS_TILE_SIZE * GIBS_ATLAS_TILE_SIZE)));
		}
	}
}