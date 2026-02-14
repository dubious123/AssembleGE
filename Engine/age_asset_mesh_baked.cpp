#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	mesh_baked
	bake_mesh(const asset::mesh_editable& mesh_edit) noexcept
	{
		auto m = asset::triangulate<vertex_fat>(mesh_edit);
		AGE_ASSERT(m.v_idx_vec.size() % 3 == 0);
		for (auto [nth, idx] : m.v_idx_vec | std::views::enumerate)
		{
			AGE_ASSERT(idx < m.vertex_vec.size());
		}

		auto&& [index_buffer, vertex_buffer] = external::meshopt::gen_remap(m.v_idx_vec, m.vertex_vec);

		external::meshopt::opt_reorder_buffers(index_buffer, vertex_buffer);

		auto vertex_buffer_quantized = cvt_vertex_to<vertex_pnt_uv1>(vertex_buffer);

		// auto vertex_buffer_quantized =
		//	vertex_buffer | std::ranges::transform(cvt_to<vertex_p_uv1>) | std::ranges::to<age::dynamic_array>();

		// auto index_buffer_quantized =
		//	index_buffer | std::ranges::transform() | std::ranges::to<age::dynamic_array>();


		//// meshopt simplify
		return {};
	}
}	 // namespace age::asset