#include "forward_plus_common.asli"

wave_size(32)
[numthreads(32, 1, 1)] void
main_cs(uint32 thread_id sv_dispatch_thread_id)

{
	const gibs_data					   data				= gibs_load_gibs_data();
	rw_stack<uint32>				   alive_atack		= gibs_load_alive_surfel_id_stack_curr(data);
	rw_byte_array<uint32>			   ray_offset_arr	= gibs_load_surfel_ray_count_prefix_rw_arr(data);
	rw_byte_array<uint32>			   ray_count_arr	= gibs_load_surfel_ray_count_ideal_rw_arr(data);
	rw_byte_array<gibs_ray_result>	   ray_res_arr		= gibs_load_ray_result_rw_arr(data);
	rw_byte_array<surfel_recycle_data> recycle_data_arr = gibs_load_surfel_recycle_data_rw_arr(data);
	rw_array<surfel>				   surfel_arr		= gibs_load_surfel_rw_arr(data);
	const uint32					   alive_count		= alive_atack.size();

	attr_branch()

	if (alive_count == 0)
	{
		return;
	}

	if (thread_id >= alive_count)
	{
		return;
	}

	const uint32 alive_id  = thread_id;
	const uint32 surfel_id = alive_atack[thread_id];

	const uint32 ray_offset = ray_offset_arr[alive_id];
	const uint32 ray_count	= ray_count_arr[alive_id];

	if (ray_count == 0) { return; }

	const bool is_new_born = recycle_data_arr[surfel_id].is_new_born();

	const uint32_2		  atlas_offset	  = gibs_calc_atlas_offset(data, surfel_id);
	rw_texture_2d<float>  luminance_atlas = global_resource_buffer[data.h_irradiance_atlas_uav_id];
	rw_texture_2d<float2> luminance_atlas = global_resource_buffer[data.h_visibility_atlas_uav_id];
	for (uint32 i = 0; i < ray_count; ++i)
	{
		const gibs_ray_result ray_res = ray_res_arr[ray_offset + i];

		// luminance
		const float3 radiance  = decode_r11g11b10(ray_res.radiance_r11g11b10);
		const float3 dir	   = decode_oct_snorm16(ray_res.dir_oct_snorm_and_extra & 0xffff);
		const float	 luminance = luminance_rec709(radiance);

		const float2  oct_uv = encode_octahedral(dir);
		const int32_2 px	 = atlas_offset
							 + int32_2(GIBS_ATLAS_TILE_SIZE / 2)
							 + int32_2(sign(oct_uv) * round(abs(oct_uv * (GIBS_ATLAS_TILE_SIZE / 2))));

		const float lum_blend_factor = is_new_born
										 ? 1.f
										 : 0.01f;

		luminance_atlas[px] = lerp(luminance_atlas[px], luminance, lum_blend_factor);

		// todo surfel irradiance update (msme 4pack)
		// change ray_res normal world -> local
	}
}