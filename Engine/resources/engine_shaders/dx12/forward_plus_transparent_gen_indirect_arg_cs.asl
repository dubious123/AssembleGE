#include "forward_plus_common.asli"

[numthreads(SORT_THREAD_COUNT, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	uint32 render_data_id = load_sort_value(dispatch_thread_id, sort_value_offset(false));

	if (render_data_id == invalid_id_uint32)
	{
		return;
	}

	store_visible_transparent_object_count_interlocked_add(1);

	transparent_object_render_data render_data = load_transparent_object_render_data(render_data_id);
	mesh_header					   msh_header  = read_mesh_header(render_data.mesh_byte_offset);

	transparent_indirect_arg arg;

	arg.object_id		 = render_data.object_id;
	arg.mesh_byte_offset = render_data.mesh_byte_offset;
	arg.thread_group_x	 = (msh_header.meshlet_count + 31) / 32;
	arg.thread_group_y	 = 1;
	arg.thread_group_z	 = 1;

	store_transparent_execute_indirect_arg(dispatch_thread_id, arg);
}