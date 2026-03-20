#include "forward_plus_common.asli"

[numthreads(TRANSPARENT_CULL_THREAD_COUNT, 1, 1)] void
main_cs(uint32 dispatch_thread_id sv_dispatch_thread_id)

{
	uint32 transparent_render_data_id = dispatch_thread_id;
	if (transparent_render_data_id < transparent_object_render_data_count)
	{
		const transparent_object_render_data render_data = load_transparent_object_render_data(transparent_render_data_id);
		const object_data					 obj_data	 = load_object_data(render_data.object_id);
		const mesh_header					 msh_header	 = read_mesh_header(render_data.mesh_byte_offset);

		const float4   quat		 = decode_quaternion(obj_data.quaternion);
		const float4x4 world_mat = affine_transformation(obj_data.pos, quat, cast<float3>(obj_data.scale));
		const float3   axis_x	 = world_mat[0].xyz;
		const float3   axis_y	 = world_mat[1].xyz;
		const float3   axis_z	 = world_mat[2].xyz;

		const float3 center_local = msh_header.aabb_min + msh_header.aabb_size * 0.5f;
		const float3 center_world = mul(world_mat, float4(center_local, 1.f)).xyz;
		const float3 extents	  = msh_header.aabb_size * 0.5f;

		bool is_visible = true;

		for (uint32 i = 0; i < 6; ++i)
		{
			const float3 plane_normal = frustum_planes[i].xyz;

			const float radius = extents.x * abs(dot(plane_normal, axis_x))
							   + extents.y * abs(dot(plane_normal, axis_y))
							   + extents.z * abs(dot(plane_normal, axis_z));

			const float distance = dot(frustum_planes[i], float4(center_world, 1.f));

			if (distance < -radius)
			{
				is_visible = false;
				break;
			}
		}

		if (is_visible)
		{
			sort_buffer[SORT_KEYS_OFFSET + transparent_render_data_id]	 = ~float_to_sortable(length(center_world - camera_pos));
			sort_buffer[SORT_VALUES_OFFSET + transparent_render_data_id] = transparent_render_data_id;
			return;
		}
	}

	sort_buffer[SORT_KEYS_OFFSET + transparent_render_data_id]	 = invalid_id_uint32;
	sort_buffer[SORT_VALUES_OFFSET + transparent_render_data_id] = invalid_id_uint32;
}