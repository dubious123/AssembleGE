#include "forward_plus_common.asli"

[numthreads(SORT_THREAD_COUNT, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	uint32 render_data_id = sort_buffer[SORT_VALUES_OFFSET + dispatch_thread_id];

	if (render_data_id == invalid_id_uint32)
	{
		return;
	}

	InterlockedAdd(frame_data_rw_buffer_uav[0].not_culled_transparent_object_count, 1);

	transparent_object_render_data render_data = load_transparent_object_render_data(render_data_id);
	mesh_header					   msh_header  = read_mesh_header(render_data.mesh_byte_offset);

	uint32 offset = TRANSPARENT_INDIRECT_ARG_OFFSET + dispatch_thread_id * 5;

	/*object_id*/
	sort_buffer[offset + 0] = render_data.object_id;

	/*mesh_byte_offset*/
	sort_buffer[offset + 1] = render_data.mesh_byte_offset;

	/*thread_group_x*/
	sort_buffer[offset + 2] = (msh_header.meshlet_count + 31) / 32;

	/*thread_group_y*/
	sort_buffer[offset + 3] = 1;

	/*thread_group_z*/
	sort_buffer[offset + 4] = 1;
}