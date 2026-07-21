#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	gibs_data										data		= gibs::load_data();
	rw_structured_buffer<gibs_tile_surfel>			surfel_arr	= global_resource_buffer[data.h_tile_surfel_buffer_uav_id];
	rw_structured_buffer<gibs_surfel_msme>			msme_arr	= global_resource_buffer[data.h_tile_surfel_msme_buffer_uav_id];
	rw_structured_buffer<gibs_tile_surfel_geometry> geo_arr		= global_resource_buffer[data.h_tile_surfel_geo_buffer_uav_id];
	rw_stack<uint32>								dead_stack	= gibs::tile::dead_id_stack(data);
	rw_stack<uint32>								alive_stack = gibs::tile::alive_id_stack_prev(data);

	const uint32 tile_sample_id = thread_id;

	if (tile_sample_id >= data.tile_count_total * GIBS_GI_RESOLVE_SAMPLE_PER_TILE)
	{
		return;
	}

	const gibs_tile_surfel_spawn_kill_data spawn_kill_data = gibs::tile::load_surfel_spawn_kill_data(data, tile_sample_id);

	if (spawn_kill_data.has_kill())
	{
		const uint32 surfel_id = spawn_kill_data.surfel_id;

		gibs_tile_surfel surfel = zero<gibs_tile_surfel>();

		surfel.kill();

		surfel_arr[surfel_id] = surfel;
	}

	// todo, maybe add some radiance sharing
	if (spawn_kill_data.has_spawn() and (gibs::debug::freeze_spawn_kill(data) is_false))
	{
		uint32 surfel_id;
		if (dead_stack.try_pop(surfel_id) is_false) { return; }

		alive_stack.push(surfel_id);
		// radiance and normal and radius will be calculate during surfel_update_cs

		gibs_tile_surfel surfel		= zero<gibs_tile_surfel>();
		surfel.irradiance_r11g11b10 = spawn_kill_data.irradiance_r11g11b10;
		surfel_arr[surfel_id]		= surfel;

		gibs_tile_surfel_geometry geo;
		geo.object_id				 = spawn_kill_data.object_id;
		geo.local_pos				 = spawn_kill_data.local_pos;
		geo.local_normal_oct_snorm16 = spawn_kill_data.local_normal_oct_snorm16;
		geo_arr[surfel_id]			 = geo;


		gibs_surfel_msme msme;
		msme.mean_long	   = decode_r11g11b10(spawn_kill_data.irradiance_r11g11b10);
		msme.mean_short	   = msme.mean_long;
		msme.vbbr		   = 0.f;
		msme.variance	   = float3(1.f, 1.f, 1.f);
		msme.inconsistency = 1.f;
		msme.incon_mean	   = 0.f;
		msme.incon_var	   = 0.f;

		msme_arr[surfel_id] = msme;

		rw_byte_array<uint16> vis_arr	  = gibs::tile::visibility_rw_arr(data, surfel_id);
		rw_byte_array<half>	  lum_arr	  = gibs::tile::luminance_rw_arr(data, surfel_id);
		rw_byte_array<half>	  lum_cdf_arr = gibs::tile::luminance_cdf_rw_arr(data, surfel_id);
		for (uint32 i = 0; i < data.atlas_texel_count(); ++i)
		{
			vis_arr.store(i, uint16(0xffffu));
			lum_arr.store(i, (1.h / half(data.atlas_texel_count())));
			lum_cdf_arr.store(i, (half(i + 1) / half(data.atlas_texel_count())));
		}
	}
}