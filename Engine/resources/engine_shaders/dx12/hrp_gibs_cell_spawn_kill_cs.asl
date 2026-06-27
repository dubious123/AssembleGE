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

	const uint32 cell_idx_flat = thread_id;

	if (cell_idx_flat >= data.cell_count_total)
	{
		return;
	}

	const gibs_cell_surfel_spawn_kill_data spawn_kill_data = gibs_load_cell_surfel_spawn_kill_data(data, cell_idx_flat);

	const byte_array<uint32> cell_to_surfel_id_arr = gibs_load_cell_to_surfel_id_arr(data);
	const gibs_cell_entry	 entry				   = gibs_load_cell_entry_arr(data)[cell_idx_flat];

	const uint32 word_begin = entry.offset / 32;
	const uint32 word_end	= (max(1u, entry.offset + entry.surfel_count()) - 1) / 32;

	const uint32 bit_begin = (entry.offset) % 32;
	const uint32 bit_end   = (entry.offset + entry.surfel_count()) % 32;

	// if (cell_idx_flat < 10)
	//{
	//	debug_log(g::fmt_gibs_spawn_kill, line, word_begin, word_end, entry.surfel_count(), cell_idx_flat);
	// }

	for (uint32 w = word_begin; w <= word_end; ++w)
	{
		uint32 ref_world_mask = ~gibs_get_cell_surfel_ref_word(data, w);

		if (w == word_begin)
		{
			ref_world_mask &= ~((1u << bit_begin) - 1);
		}
		if (w == word_end and bit_end != 0)
		{
			ref_world_mask &= ((1u << bit_end) - 1);
		}

		while (ref_world_mask != 0)
		{
			const uint32 bit	   = first_bit_low(ref_world_mask);
			const uint32 slot_idx  = w * 32 + bit;
			ref_world_mask		  &= ~(1u << bit);

			const uint32 surfel_id = cell_to_surfel_id_arr[slot_idx];

			gibs_set_surfel_ref(data, surfel_id);
		}

		break;
	}

	if (spawn_kill_data.has_kill())
	{
		const uint32 surfel_id = spawn_kill_data.surfel_id;

		surfel surfel = surfel_arr[surfel_id];

		surfel.kill();

		surfel_arr.store(surfel_id, surfel);
	}

	if (spawn_kill_data.has_spawn() and ((data.debug_flags & GIBS_DEBUG_FLAGS_FREEZE_SPAWN) == 0))
	{
		uint32 surfel_id;
		if (dead_stack.try_pop(surfel_id) is_false) { return; }

		alive_stack.push(surfel_id);
		// radiance and normal and radius will be calculate during surfel_update_cs

		surfel surfel = surfel_arr[surfel_id];

		surfel.radiance_r11g11b10 = spawn_kill_data.radiance_r11g11b10;
		surfel.recycle_data		  = 0u;

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