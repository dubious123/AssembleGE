#include "forward_plus_common.asli"

bool
is_visible(const object_data obj_data, const meshlet_header m_header)
{
	const float4 quaternion = decode_quaternion(obj_data.quaternion);
	const float3 scale		= cast<float3>(obj_data.scale);
	const float3 pos		= obj_data.pos;

	// 1. frustum culling using meshlet shpere

	const float3 sphere_center =
		rotate((cast<float3>(m_header.aabb_min) + cast<float3>(m_header.aabb_size) * 0.5f) * scale, quaternion)
		+ pos;

	const float sphere_radius = length(m_header.aabb_size * 0.5f) * max(max(scale.x, scale.y), scale.z);

	expand(6)

	for (uint32 i = 0; i < 6; ++i)
	{
		if (calc_point_to_plane_distance(sphere_center, frustum_planes[i]) < -sphere_radius)
		{
			return false;
		}
	}

	// 2. cone culling
	const float3 cone_axis_world = rotate(decode_oct_snorm(m_header.cone_axis_oct), quaternion);
	const float	 cone_cutoff	 = snorm8_to_float(m_header.cone_cull_cutoff_and_extra & 0xffu);
	const float3 center_to_cam	 = sphere_center - camera_pos;
	const float	 dist			 = length(center_to_cam);

	if (dot(center_to_cam, cone_axis_world) >= cone_cutoff * dist + sphere_radius)
	{
		return false;
	}

	return true;
}

groupshared opaque_as_to_ms as_out;

[numthreads(32, 1, 1)] void
main_as(
	uint32_3 dispatch_thread_id sv_dispatch_thread_id,
	uint32_3 group_id sv_group_id,
	uint32_3 group_thread_id sv_group_thread_id)

{
	const uint32 render_data_id = dispatch_thread_id.x;

	bool visible = false;

	if (render_data_id < opaque_meshlet_render_data_count)
	{
		const opaque_meshlet_render_data render_data = load_opaque_meshlet_render_data(render_data_id);
		const object_data				 obj_data	 = load_object_data(render_data.object_id);

		const uint32 meshlet_idx = render_data.meshlet_id;

		const mesh_header	 msh_header	  = read_mesh_header(render_data.mesh_byte_offset);
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