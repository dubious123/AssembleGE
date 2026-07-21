#include "hrp_common.asli"

wave_size(AGE_WAVE_SIZE)
[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data									data			   = gibs::load_data();
	rw_structured_buffer<gibs_cell_surfel>			surfel_arr		   = global_resource_buffer[data.h_cell_surfel_buffer_uav_id];
	rw_structured_buffer<gibs_surfel_msme>			surfel_msme_arr	   = global_resource_buffer[data.h_cell_surfel_msme_buffer_uav_id];
	rw_structured_buffer<gibs_cell_surfel_geometry> surfel_geo_arr	   = global_resource_buffer[data.h_cell_surfel_geo_buffer_uav_id];
	rw_stack<uint32>								surfel_dead_stack  = gibs::cell::dead_id_stack(data);
	rw_stack<uint32>								surfel_alive_stack = gibs::cell::alive_id_stack_prev(data);

	rw_structured_buffer<gibs_surfel_probe>			 probe_arr		   = global_resource_buffer[data.h_surfel_probe_buffer_uav_id];
	rw_structured_buffer<gibs_surfel_probe_msme>	 probe_msme_arr	   = global_resource_buffer[data.h_surfel_probe_msme_buffer_uav_id];
	rw_structured_buffer<gibs_surfel_probe_geometry> probe_geo_arr	   = global_resource_buffer[data.h_surfel_probe_geo_buffer_uav_id];
	rw_structured_buffer<gibs_recycle_data>			 probe_recycle_arr = global_resource_buffer[data.h_surfel_probe_recycle_buffer_uav_id];
	rw_stack<uint32>								 probe_dead_stack  = gibs::probe::dead_id_stack(data);
	rw_stack<uint32>								 probe_alive_stack = gibs::probe::alive_id_stack_prev(data);

	structured_buffer<gibs_ray_hit_result>		ray_hit_buffer		= global_resource_buffer[data.h_ray_hit_buffer_srv_id];
	structured_buffer<gibs_ray_lighting_result> ray_lighting_buffer = global_resource_buffer[data.h_ray_lighting_buffer_srv_id];

	const uint32 cell_id = thread_id;

	if (cell_id >= data.cell_count_total)
	{
		return;
	}

	// inc probe ref
	{
		const byte_array<uint32>	cell_to_probe_id_arr = gibs::cell::cell_to_probe_id_arr(data);
		const gibs_cell_probe_entry entry				 = gibs::cell::probe_entry_arr(data)[cell_id];

		const uint32 word_begin = entry.offset / 32;
		const uint32 word_end	= (max(1u, entry.offset + entry.probe_count) - 1) / 32;

		const uint32 bit_begin = (entry.offset) % 32;
		const uint32 bit_end   = (entry.offset + entry.probe_count) % 32;

		for (uint32 w = word_begin; w <= word_end; ++w)
		{
			uint32 ref_world_mask = ~gibs::cell::get_probe_ref_word(data, w);

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

				const uint32 probe_id = cell_to_probe_id_arr[slot_idx];

				gibs::set_probe_ref(data, probe_id);
			}
		}
	}

	{
		const byte_array<uint32>	 cell_to_surfel_id_arr = gibs::cell::cell_to_surfel_id_arr(data);
		const gibs_cell_surfel_entry entry				   = gibs::cell::surfel_entry_arr(data)[cell_id];

		const uint32 word_begin = entry.offset / 32;
		const uint32 word_end	= (max(1u, entry.offset + entry.surfel_count) - 1) / 32;

		const uint32 bit_begin = (entry.offset) % 32;
		const uint32 bit_end   = (entry.offset + entry.surfel_count) % 32;

		for (uint32 w = word_begin; w <= word_end; ++w)
		{
			uint32 ref_world_mask = ~gibs::cell::get_surfel_ref_word(data, w);

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

				gibs::set_cell_surfel_ref(data, surfel_id);
			}
		}
	}

	// probe spawn
	{
		const uint32 ray_id = gibs::cell::load_probe_spawn_data(data, cell_id);

		if (gibs::debug::freeze_spawn_kill(data) is_false and (ray_id != invalid_id_uint32))
		{
			uint32 probe_id;
			if (probe_dead_stack.try_pop(probe_id))
			{
				probe_alive_stack.push(probe_id);
				// radiance and normal and radius will be calculate during probe_update_cs

				gibs_ray_hit_result ray_hit	 = ray_hit_buffer[ray_id];
				const float3		radiance = decode_r11g11b10(ray_lighting_buffer[ray_id].radiance_r11g11b10);

				gibs_surfel_probe probe = zero<gibs_surfel_probe>();
				probe.set_new_born();

				// eval(depth_sh, any_dir) == 1.f
				probe.coverage_far_sh = half4(0.h, 0.h, 0.h, 0.h);
				probe.depth_sh		  = half4(half(1.f / 0.28209479177387814347403972578039f), 0.h, 0.h, 0.h);

				const float3 dir_local	  = decode_world_hemi_oct_snorm8(uint32_lower_to_uint16(ray_hit.dir_oct_snorm8));
				const float	 cos_theta	  = dir_local.y;
				const float	 contribution = cos_theta / max(epsilon_1e6, ray_hit.pdf);

				probe.irradiance_r11g11b10 = encode_r11g11b10(radiance * contribution);

				probe_arr[probe_id] = probe;

				gibs_surfel_probe_msme msme;
				msme.mean_long	   = radiance * contribution;
				msme.mean_short	   = msme.mean_long;
				msme.vbbr		   = 0.f;
				msme.variance	   = float3(1.f, 1.f, 1.f);
				msme.inconsistency = 1.f;

				probe_msme_arr[probe_id] = msme;

				gibs_surfel_probe_geometry geo;
				geo.object_id			= ray_hit.object_id;
				geo.primitive_id		= ray_hit.primitive_id;
				geo.barycentric_unorm16 = ray_hit.barycentric_unorm16;
				probe_geo_arr[probe_id] = geo;

				probe_recycle_arr[probe_id] = zero<gibs_recycle_data>();
			}
		}
	}

	// probe kill
	{
		const uint32 kill_probe_id = gibs::cell::load_probe_kill_data(data, cell_id);
		if (gibs::debug::freeze_spawn_kill(data) is_false and (kill_probe_id != invalid_id_uint32))
		{
			gibs_surfel_probe_geometry geo = zero<gibs_surfel_probe_geometry>();
			geo.kill();
			probe_geo_arr[kill_probe_id] = geo;
		}
	}

	// cell surfel spawn
	{
		const uint32 ray_id = gibs::cell::load_surfel_spawn_data(data, cell_id);

		if (gibs::debug::freeze_spawn_kill(data) is_false and (ray_id != invalid_id_uint32))
		{
			uint32 surfel_id;
			if (surfel_dead_stack.try_pop(surfel_id))
			{
				surfel_alive_stack.push(surfel_id);
				// radiance and normal and radius will be calculate during surfel_update_cs

				gibs_ray_hit_result ray_hit	 = ray_hit_buffer[ray_id];
				const float3		radiance = decode_r11g11b10(ray_lighting_buffer[ray_id].radiance_r11g11b10);

				const float3 dir_local	  = decode_world_hemi_oct_snorm8(uint32_lower_to_uint16(ray_hit.dir_oct_snorm8));
				const float	 cos_theta	  = dir_local.y;
				const float	 contribution = cos_theta / max(epsilon_1e6, ray_hit.pdf);

				gibs_cell_surfel surfel	  = zero<gibs_cell_surfel>();
				surfel.radiance_r11g11b10 = encode_r11g11b10(radiance * contribution);
				surfel_arr[surfel_id]	  = surfel;

				gibs_cell_surfel_geometry geo;
				geo.object_id			  = ray_hit.object_id;
				geo.primitive_id		  = ray_hit.primitive_id;
				geo.barycentric_unorm16	  = ray_hit.barycentric_unorm16;
				surfel_geo_arr[surfel_id] = geo;


				gibs_surfel_msme msme;
				msme.mean_long	   = radiance * contribution;
				msme.mean_short	   = msme.mean_long;
				msme.vbbr		   = 0.f;
				msme.variance	   = float3(1.f, 1.f, 1.f);
				msme.inconsistency = 1.f;
				msme.incon_mean	   = 0.f;
				msme.incon_var	   = 0.f;

				surfel_msme_arr[surfel_id] = msme;

				rw_byte_array<uint16> vis_arr	  = gibs::cell::visibility_rw_arr(data, surfel_id);
				rw_byte_array<half>	  lum_arr	  = gibs::cell::luminance_rw_arr(data, surfel_id);
				rw_byte_array<half>	  lum_cdf_arr = gibs::cell::luminance_cdf_rw_arr(data, surfel_id);
				for (uint32 i = 0; i < data.atlas_texel_count(); ++i)
				{
					vis_arr.store(i, uint16(0xffffu));
					lum_arr.store(i, (1.h / half(data.atlas_texel_count())));
					lum_cdf_arr.store(i, (half(i + 1) / half(data.atlas_texel_count())));
				}
			}
		}
	}

	// surfel kill
	{
		const uint32 kill_surfel_id = gibs::cell::load_surfel_kill_data(data, cell_id);
		if (gibs::debug::freeze_spawn_kill(data) is_false and (kill_surfel_id != invalid_id_uint32))
		{
			gibs_cell_surfel_geometry geo = zero<gibs_cell_surfel_geometry>();
			geo.kill();
			surfel_geo_arr[kill_surfel_id] = geo;
		}
	}
}