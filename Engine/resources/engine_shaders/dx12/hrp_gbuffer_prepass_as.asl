#include "hrp_common.asli"

groupshared opaque_as_to_ms as_out;

[numthreads(32, 1, 1)] void
main_as(
	uint32_3 dispatch_thread_id sv_dispatch_thread_id,
	uint32_3 group_id			sv_group_id,
	uint32_3 group_thread_id	sv_group_thread_id)

{
	const uint32 render_data_id = dispatch_thread_id.x;

	bool visible = false;

	if (render_data_id < opaque_meshlet_render_data_count)
	{
		const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_data_id);
		const object_data				 obj_data	 = load_object_data(render_data.object_id);

		const uint32 meshlet_idx = render_data.meshlet_id;

		const mesh_header	 msh_header	  = read_mesh_header<opaque_meshlet_render_data>(render_data);
		const meshlet_header mshlt_header = read_meshlet_header(msh_header, meshlet_idx);

		visible = is_visible(obj_data, mshlt_header);
	}

	const uint32_4 ballot		= wave_active_ballot(visible);
	uint32		   visible_mask = ballot.x;

	if (wave_is_first_lane())
	{
		as_out.meshlet_32_group_idx = group_id.x;
		as_out.meshlet_alive_mask	= visible_mask;
	}

	dispatch_mesh(countbits(visible_mask), 1, 1, as_out);
}