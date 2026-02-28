#pragma once

namespace age::asset
{
	struct vertex_fat
	{
		float3				  pos	  = {};
		float3				  normal  = {};
		float4				  tangent = {};
		std::array<float2, 4> uv_set  = {};
	};

	template <typename t_vertex>
	struct mesh_triangulated
	{
		data_structure::vector<t_vertex> vertex_vec{};
		data_structure::vector<uint32>	 v_idx_vec{};
	};

	struct meshlet_header
	{
		oct<int8> cone_axis_oct;
		int8	  cone_cull_cutoff;
		uint8	  padding;	   // apex = center - axis * offset;

		int16_3	 aabb_min;	   // 6byte
		uint16_3 aabb_size;	   // 6byte
	};

	struct meshlet
	{
		uint32 global_index_offset{};
		uint32 primitive_offset{};

		uint8  vertex_count{};
		uint8  primitive_count{};
		uint16 padding{};
	};
}	 // namespace age::asset
