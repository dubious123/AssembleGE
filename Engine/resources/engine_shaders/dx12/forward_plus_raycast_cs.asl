#include "forward_plus_common.asli"

[numthreads(32, 1, 1)] void
main_cs(uint32_3 thread_id sv_dispatch_thread_id)

{
	if (thread_id.x >= rt_raycast_request_count) { return; }

	const raycast_request req = load_rt_raycast_request(thread_id.x);

	raycast_result res;
	res.object_id = invalid_id_uint32;

	rt_acceleration_structure tlas = global_resource_buffer[rt_tlas_buffer_id];

	ray_desc desc;
	desc.Origin	   = req.origin;
	desc.Direction = req.dir;
	desc.TMin	   = 0;
	desc.TMax	   = req.t_max;

	ray_query<RAY_FLAG_FORCE_OPAQUE> query;

	rt_trace_ray_inline(query, tlas, RAY_FLAG_NONE, req.mask_and_extra & RT_MASK_ALL, desc);

	while (rt_proceed(query)) { }

	if (rt_committed_status(query) == COMMITTED_TRIANGLE_HIT)
	{
		const uint32 rt_instance_id = rt_committed_instance_id(query);

		const rt_instance_render_data render_data = load_rt_instance_render_data(rt_instance_id);
		res.t_hit								  = rt_committed_ray_t(query);
		res.object_id							  = render_data.object_id;
		res.world_pos							  = req.origin + res.t_hit * req.dir;
	}

	store_rt_raycast_result(thread_id.x, res);
}