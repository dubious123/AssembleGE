#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gist_data									data			   = gist::load_data();
	rw_structured_buffer<gist_cell_surfel>			surfel_arr		   = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];
	rw_structured_buffer<gist_surfel_msme>			surfel_msme_arr	   = global_resource_buffer[data.h_cell_surfel_msme_buffer_uav_id];
	rw_structured_buffer<gist_cell_surfel_geometry> surfel_geo_arr	   = global_resource_buffer[data.h_cell_surfel_geo_buffer_uav_id];
	rw_stack<uint32>								surfel_dead_stack  = gist::cell::dead_id_stack(data);
	rw_stack<uint32>								surfel_alive_stack = gist::cell::alive_id_stack_prev(data);

	structured_buffer<gist_ray_hit_result>		ray_hit_buffer		= global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gist_ray_lighting_result> ray_lighting_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	const uint32 cell_id = thread_id;

	if (cell_id >= data.cell_count_total())
	{
		return;
	}

	{
		const byte_array<uint32>	 cell_to_surfel_id_arr = gist::cell::cell_to_surfel_id_arr(data);
		const gist_cell_surfel_entry entry				   = gist::cell::surfel_entry_arr(data)[cell_id];

		const uint32 word_begin = entry.offset / 32;
		const uint32 word_end	= (max(1u, entry.offset + entry.surfel_count) - 1) / 32;

		const uint32 bit_begin = (entry.offset) % 32;
		const uint32 bit_end   = (entry.offset + entry.surfel_count) % 32;

		for (uint32 w = word_begin; w <= word_end; ++w)
		{
			uint32 ref_world_mask = ~gist::cell::get_surfel_ref_word(data, w);

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

				gist::set_cell_surfel_ref(data, surfel_id);
			}
		}
	}

	// cell surfel spawn
	{
		const uint32 ray_id = gist::cell::load_surfel_spawn_data(data, cell_id);

		if (ray_id != invalid_id_uint32)
		{
			attr_branch()

			if (gist::debug::freeze_cell_surfel_spawn(data) is_false)
			{
				uint32 surfel_id;
				if (surfel_dead_stack.try_pop(surfel_id))
				{
					surfel_alive_stack.push(surfel_id);

					gist_ray_hit_result ray_hit = ray_hit_buffer[ray_id];

					const float3 dir_local	  = decode_world_hemi_oct_snorm8(uint32_lower_to_uint16(ray_hit.dir_oct_snorm8));
					const float	 cos_theta	  = dir_local.y;
					const float	 contribution = cos_theta / max(epsilon_1e6, ray_hit.pdf);

					gist_cell_surfel surfel		= zero<gist_cell_surfel>();
					surfel.irradiance_r11g11b10 = ray_lighting_buffer[ray_id].irradiance_r11g11b10;
					surfel_arr[surfel_id]		= surfel;

					gist_cell_surfel_geometry geo;
					geo.object_id			  = ray_hit.object_id;
					geo.primitive_id		  = ray_hit.primitive_id;
					geo.barycentric_unorm16	  = ray_hit.barycentric_unorm16;
					surfel_geo_arr[surfel_id] = geo;


					gist_surfel_msme msme;
					msme.mean_long	   = decode_r11g11b10(ray_lighting_buffer[ray_id].irradiance_r11g11b10);
					msme.mean_short	   = msme.mean_long;
					msme.vbbr		   = 0.f;
					msme.variance	   = float3(1.f, 1.f, 1.f);
					msme.inconsistency = 1.f;
					msme.incon_mean	   = 0.f;
					msme.incon_var	   = 0.f;

					surfel_msme_arr[surfel_id] = msme;

					rw_byte_array<uint16> vis_arr	  = gist::cell::visibility_rw_arr(data, surfel_id);
					rw_byte_array<half>	  lum_arr	  = gist::cell::luminance_rw_arr(data, surfel_id);
					rw_byte_array<half>	  lum_cdf_arr = gist::cell::luminance_cdf_rw_arr(data, surfel_id);
					for (uint32 i = 0; i < data.atlas_texel_count(); ++i)
					{
						vis_arr.store(i, uint16(0));
						// vis_arr.store(i, uint16(0xffffu));
						lum_arr.store(i, (1.h / half(data.atlas_texel_count())));
						lum_cdf_arr.store(i, (half(i + 1) / half(data.atlas_texel_count())));
					}
				}
			}
		}
	}

	// surfel kill
	{
		const uint32 kill_surfel_id = gist::cell::load_surfel_kill_data(data, cell_id);
		if (kill_surfel_id != invalid_id_uint32)
		{
			attr_branch()

			if (gist::debug::freeze_cell_surfel_kill(data) is_false)
			{
				gist_cell_surfel_geometry geo = zero<gist_cell_surfel_geometry>();
				geo.kill();
				surfel_geo_arr[kill_surfel_id] = geo;
			}
		}
	}
}