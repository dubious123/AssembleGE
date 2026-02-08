namespace age::asset
{
	mesh_baked
	bake_mesh(asset::mesh_editable& m) noexcept
	{
		auto v_idx_vec = data_structure::vector<uint32>{};
		{
			v_idx_vec.resize(m.vertex_vec.size(), 0);
		}

		auto stream_arr = std::array{
			external::meshopt::data_stream{ .data = m.position_vec.data(), .size = sizeof(float3), .stride = sizeof(float3) },
			external::meshopt::data_stream{ .data = m.vertex_attr_vec.data(), .size = sizeof(mesh_editable::vertex_attribute), .stride = sizeof(mesh_editable::vertex_attribute) },
		};

		auto new_vertex_count = external::meshopt::generateVertexRemap(
			v_idx_vec.data(),
			nullptr,
			m.vertex_vec.size(),
			m.vertex_vec.data(),
			m.vertex_vec.size(),
			sizeof(m.vertex_vec[0]));

		auto new_vertex_vec = data_structure::vector<float3>{};
		{
			new_vertex_vec.resize(new_vertex_count);
		}

		auto new_vertex_idx_vec = data_structure::vector<uint32>();
		{
			new_vertex_idx_vec.resize(new_vertex_count);
		}

		v_idx_vec.resize(new_vertex_count);

		external::meshopt::remapIndexBuffer();
		external::meshopt::remapVertexBuffer();

		return {};
	}
}	 // namespace age::asset