#include "hrp_common.asli"

[numthreads(AGE_WAVE_SIZE, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	const gist_data data = gist::load_data();

	if (dispatch_thread_id < data.max_cell_surfel_count)
	{
		rw_stack<uint32> cell_surfel_dead_stack = gist::cell::dead_id_stack(data);
		cell_surfel_dead_stack.resize(data.max_cell_surfel_count);
		cell_surfel_dead_stack.set(dispatch_thread_id, dispatch_thread_id);
	}

	const int32_2 px_luminance_extent = ceil(int32_2(backbuffer_size), GIST_PX_LUMINANCE_TILE_SIZE);

	if (dispatch_thread_id < px_luminance_extent.x * px_luminance_extent.y)
	{
		const int32_2 px = int32_2((dispatch_thread_id % px_luminance_extent.x), (dispatch_thread_id / px_luminance_extent.x))
						 * GIST_PX_LUMINANCE_TILE_SIZE;

		rw_byte_array<half> lum_arr		= gist::tile::px_luminance_rw_arr(data, px);
		rw_byte_array<half> lum_cdf_arr = gist::tile::px_luminance_cdf_rw_arr(data, px);

		for (uint32 i = 0; i < data.atlas_texel_count(); ++i)
		{
			lum_arr.store(i, (1.h / half(data.atlas_texel_count())));
			lum_cdf_arr.store(i, (half(i + 1) / half(data.atlas_texel_count())));
		}
	}

	if (dispatch_thread_id.x == 0)
	{
		gist::cell::alive_id_stack_curr(data).resize(0u);
		gist::cell::alive_id_stack_prev(data).resize(0u);
	}
}