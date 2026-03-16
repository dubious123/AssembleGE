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

	const float sphere_radius = length(cast<float3>(m_header.aabb_size) * 0.5f) * max(max(scale.x, scale.y), scale.z);

	expand(6)

	for (uint32 i = 0; i < 6; ++i)
	{
		if (calc_point_to_plane_distance(sphere_center, frustum_planes[i]) < -sphere_radius)
		{
			return false;
		}
	}

	return true;
}

groupshared
	opaque_as_to_ms as_out;

[numthreads(32, 1, 1)] void
main_as(
	uint32 meshlet_idx	   sv_dispatch_thread_id,
	uint32 group_id		   sv_group_id,
	uint32 group_thread_id sv_group_thread_id)

{
	const uint32	  object_id		   = arg0;
	const uint32	  mesh_byte_offset = arg1;
	const mesh_header msh_header	   = read_mesh_header(mesh_byte_offset);

	bool visible = false;

	if (meshlet_idx < msh_header.meshlet_count)
	{
		const object_data obj_data = object_data_buffer[object_id];

		const meshlet_header mshlt_header = read_meshlet_header(msh_header, meshlet_idx);

		visible = is_visible(obj_data, mshlt_header);
	}

	const uint32_4 ballot		= wave_active_ballot(visible);
	const uint32   visible_mask = ballot.x;


	if (wave_is_first_lane())
	{
		as_out.meshlet_32_group_idx = group_id;
		as_out.meshlet_alive_mask	= visible_mask;
	}

	dispatch_mesh(countbits(visible_mask), 1, 1, as_out);
}