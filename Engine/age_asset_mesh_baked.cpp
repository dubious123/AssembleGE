#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	mesh_baked
	bake_mesh(const asset::mesh_editable& mesh_edit) noexcept
	{
		auto m = asset::triangulate<asset::vertex_fat>(mesh_edit);
		AGE_ASSERT(m.v_idx_vec.size() % 3 == 0);
		for (auto [nth, idx] : m.v_idx_vec | std::views::enumerate)
		{
			AGE_ASSERT(idx < m.vertex_vec.size());
		}

		auto v_idx_remap_vec = data_structure::vector<uint32>::gen_reserved(m.v_idx_vec.size());

		auto new_vertex_count = external::meshopt::generateVertexRemap(
			v_idx_remap_vec.data(),
			m.v_idx_vec.data(),
			m.v_idx_vec.size(),
			m.vertex_vec.data(),
			m.vertex_vec.size(),
			sizeof(m.vertex_vec[0]));

		//// meshopt_generateVertexRemapMulti


		auto new_idx_vec	= data_structure::vector<uint32>::gen_reserved(v_idx_remap_vec.size());
		auto new_vertex_vec = data_structure::vector<asset::vertex_fat>::gen_reserved(new_vertex_count);
		// external::meshopt::remapIndexBuffer(
		//	new_idx_vec.data(),
		//	m.v_idx_vec.data(), m.v_idx_vec.size(),
		//	v_idx_remap_vec.data());
		// external::meshopt::remapVertexBuffer(
		//	new_vertex_vec.data(),
		//	m.vertex_vec.data(), m.vertex_vec.size(),
		//	sizeof(asset::vertex_fat),
		//	v_idx_remap_vec.data());

		// external::meshopt::optimizeVertexCache(
		//	new_idx_vec.data(),
		//	new_idx_vec.data(),
		//	new_idx_vec.size(),
		//	new_vertex_vec.size());	   // reorder vertex index

		// external::meshopt::optimizeVertexCacheStrip(
		//	new_idx_vec.data(),
		//	new_idx_vec.data(),
		//	new_idx_vec.size(),
		//	new_vertex_vec.size());	   // trades off some efficiency in vertex transform for smaller index (and sometimes vertex) data.

		// external::meshopt::optimizeOverdraw(
		//	new_idx_vec.data(),
		//	new_idx_vec.data(),
		//	new_idx_vec.size(),
		//	&new_vertex_vec[0].pos.x,
		//	new_vertex_vec.size(),
		//	sizeof(new_vertex_vec[0]),
		//	1.05f);	   // reorder vertex index (reduce redraw), optional, may not beneficial

		// external::meshopt::optimizeVertexFetch(
		//	new_vertex_vec.data(),
		//	new_idx_vec.data(),
		//	new_idx_vec.size(),
		//	new_vertex_vec.data(),
		//	new_vertex_vec.size(),
		//	sizeof(new_vertex_vec[0]));	   // reorder_vertex_vec

		// auto shadow_idx_vec = data_structure::vector<uint32>{};
		//{
		//	shadow_idx_vec.resize(new_idx_vec.size());
		//	external::meshopt::generateShadowIndexBuffer(
		//		shadow_idx_vec.data(),
		//		new_idx_vec.data(),
		//		new_idx_vec.size(),
		//		new_vertex_vec.data(),
		//		new_vertex_vec.size(),
		//		sizeof(new_vertex_vec[0].pos),
		//		sizeof(new_vertex_vec[0]));
		//	external::meshopt::optimizeVertexCache(
		//		shadow_idx_vec.data(),
		//		shadow_idx_vec.data(),
		//		shadow_idx_vec.size(),
		//		new_vertex_vec.size());
		// }

		// auto max_vertex_per_meshlet	  = 64;
		// auto max_triangle_per_meshlet = 126;
		// auto min_triangle_per_meshlet = max_triangle_per_meshlet / 2;
		// auto cone_weight			  = 0.25f;
		// auto max_meshlet_count =
		//	external::meshopt::buildMeshletsBound(
		//		new_idx_vec.size(),
		//		max_vertex_per_meshlet,
		//		max_triangle_per_meshlet);

		// auto meshlet_vec			= data_structure::vector<external::meshopt::meshlet>{};
		// auto meshlet_global_idx_vec = data_structure::vector<uint32>{};
		// auto meshlet_local_idx_vec	= data_structure::vector<uint8>{};
		//{
		//	meshlet_vec.resize(max_meshlet_count);
		//	meshlet_global_idx_vec.resize(new_idx_vec.size());
		//	meshlet_local_idx_vec.resize(new_idx_vec.size());	 // vertex = nth | local_idx_vec | global_idx_vec | vertex_vec

		//	auto meshlet_count = external::meshopt::buildMeshlets(
		//		meshlet_vec.data(),
		//		meshlet_global_idx_vec.data(),
		//		meshlet_local_idx_vec.data(),
		//		new_idx_vec.data(),
		//		new_idx_vec.size(),
		//		&new_vertex_vec[0].pos.x,
		//		new_vertex_vec.size(),
		//		sizeof(new_vertex_vec[0]),
		//		max_triangle_per_meshlet / 2,
		//		max_triangle_per_meshlet,
		//		cone_weight);

		//	{

		//		auto new_max_meshlet_count =
		//			external::meshopt::buildMeshletsBound(
		//				new_idx_vec.size(),
		//				max_vertex_per_meshlet,
		//				min_triangle_per_meshlet);
		//		meshlet_vec.resize(new_max_meshlet_count);

		//		auto split_factor = 2.0f;
		//		external::meshopt::buildMeshletsFlex(
		//			meshlet_vec.data(),
		//			meshlet_global_idx_vec.data(),
		//			meshlet_local_idx_vec.data(),
		//			new_idx_vec.data(),
		//			new_idx_vec.size(),
		//			&new_vertex_vec[0].pos.x,
		//			new_vertex_vec.size(),
		//			sizeof(new_vertex_vec[0]),
		//			max_vertex_per_meshlet,
		//			max_triangle_per_meshlet / 4,
		//			max_triangle_per_meshlet,
		//			cone_weight,
		//			split_factor);	  // for lod
		//	}
		//	{
		//		c_auto fill_weight = 0.5f;

		//		auto new_max_meshlet_count =
		//			external::meshopt::buildMeshletsBound(
		//				new_idx_vec.size(),
		//				max_vertex_per_meshlet,
		//				min_triangle_per_meshlet);
		//		meshlet_vec.resize(new_max_meshlet_count);

		//		auto special_meshlet_count =
		//			external::meshopt::buildMeshletsSpatial(
		//				meshlet_vec.data(),
		//				meshlet_global_idx_vec.data(),
		//				meshlet_local_idx_vec.data(),
		//				new_idx_vec.data(),
		//				new_idx_vec.size(),
		//				&new_vertex_vec[0].pos.x,
		//				new_vertex_vec.size(),
		//				sizeof(new_vertex_vec[0]),
		//				max_vertex_per_meshlet,
		//				min_triangle_per_meshlet,
		//				max_triangle_per_meshlet,
		//				fill_weight /*0(smaller clusters) ~ 1.f (triangle utilization)*/);	  // for raycast
		//	}

		//	{

		//		c_auto& meshlet_last = meshlet_vec[meshlet_count - 1];
		//		meshlet_global_idx_vec.resize(meshlet_last.vertex_offset + meshlet_last.vertex_count);
		//		meshlet_local_idx_vec.resize(meshlet_last.triangle_offset + meshlet_last.triangle_count * 3);
		//		meshlet_vec.resize(meshlet_count);
		//	}

		//	for (c_auto& m : meshlet_vec)
		//	{
		//		external::meshopt::optimizeMeshlet(
		//			meshlet_global_idx_vec.data() + m.vertex_offset,
		//			meshlet_local_idx_vec.data() + m.triangle_offset,
		//			m.triangle_count,
		//			m.vertex_count);

		//		auto b = external::meshopt::computeMeshletBounds(
		//			meshlet_global_idx_vec.data() + m.vertex_offset,
		//			meshlet_local_idx_vec.data() + m.triangle_offset,
		//			m.triangle_count,
		//			&new_vertex_vec[0].pos.x,
		//			new_vertex_vec.size(),
		//			sizeof(new_vertex_vec[0]));

		//		// if (dot(normalize(cone_apex - camera_position), cone_axis) >= cone_cutoff) reject();
		//	}
		//}


		//// decoding, encoding
		//{
		//	auto v_buffer = data_structure::vector<uint8>();
		//	v_buffer.resize(external::meshopt::encodeVertexBufferBound(new_vertex_vec.size(), sizeof(new_vertex_vec[0])));
		//	external::meshopt::encodeVertexBuffer(
		//		v_buffer.data(),
		//		v_buffer.size(),
		//		new_vertex_vec.data(),
		//		new_vertex_vec.size(),
		//		sizeof(new_vertex_vec[0]));	   // assume vertex fetched, if not, use meshopt_spatialSortRemap

		//	auto decode_res = external::meshopt::decodeVertexBuffer(
		//		new_vertex_vec.data(),
		//		new_vertex_vec.size(),
		//		sizeof(new_vertex_vec[0]),	   // assert sizeof(stride) % 4 == 0
		//		v_buffer.data(),
		//		v_buffer.size());
		//	AGE_ASSERT(decode_res == 0);
		//}
		//{
		//	auto i_buffer = data_structure::vector<uint8>();
		//	i_buffer.resize(external::meshopt::encodeIndexBufferBound(new_idx_vec.size(), new_vertex_vec.size()));
		//	i_buffer.resize(external::meshopt::encodeIndexBuffer(i_buffer.data(), i_buffer.size(), new_idx_vec.data(), new_idx_vec.size()));

		//	auto decode_res = external::meshopt::decodeIndexBuffer(
		//		new_idx_vec.data(),
		//		new_idx_vec.size(),
		//		sizeof(new_idx_vec[0]),
		//		i_buffer.data(),
		//		i_buffer.size());
		//	AGE_ASSERT(decode_res == 0);
		//}
		//{
		//	auto m_buffer = data_structure::vector<uint8>();
		//	m_buffer.resize(external::meshopt::encodeMeshletBound(
		//		max_vertex_per_meshlet,
		//		max_triangle_per_meshlet));

		//	for (const auto& m : meshlet_vec)
		//	{
		//		auto msize = external::meshopt::encodeMeshlet(
		//			m_buffer.data(),
		//			m_buffer.size(),
		//			meshlet_global_idx_vec.data() + m.vertex_offset,
		//			m.vertex_count,
		//			meshlet_local_idx_vec.data() + m.triangle_offset,
		//			m.triangle_count);

		//		// write m.vertex_count, m.triangle_count, msize, m_buffer[0..msize-1] to output stream;

		//		auto* p_vertice_vec	 = (uint16*)nullptr;
		//		auto* p_triangle_vec = (uint8*)nullptr;

		//		// auto res =
		//		//	external::meshopt::decodeMeshletRaw(
		//		//		p_vertice_vec,
		//		//		m.vertex_count,
		//		//		p_triangle_vec,
		//		//		m.triangle_count,
		//		//		m_buffer.data(),
		//		//		m_buffer.size());	 // for simd
		//		// AGE_ASSERT(res == 0);
		//	}
		//}

		//// meshopt simplify
		return {};
	}
}	 // namespace age::asset