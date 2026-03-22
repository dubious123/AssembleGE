#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::earcut
{
	data_structure::vector<uint32>
	perform(const asset::mesh_editable& m) noexcept
	{
		auto idx_vec = age::vector<uint32>::gen_reserved(
			std::ranges::fold_left(
				m.face_vec | std::views::transform([&m](c_auto& f) { return std::ranges::distance(m.vertex_view(f)) * 3; }),
				0ull,
				std::plus{}));

		auto corner_pos_arr = dynamic_array<std::pair<float, float>>::gen_sized_default(m.boundary_to_vertex_idx_vec.size());

		for (auto& f : m.face_vec)
		{
			c_auto	offset = m.outer_boundary(f).to_vertex_idx_offset;
			c_auto& origin = m.position_vec[m.vertex_vec[m.boundary_to_vertex_idx_vec[offset]].pos_idx];

			auto corner_span_ptr_arr   = dynamic_array<std::pair<float, float>*>::gen_sized_default(f.to_boundary_idx_count);
			auto corner_span_count_arr = dynamic_array<uint32>::gen_sized_default(f.to_boundary_idx_count);

			for (auto&& [nth_boundary, b] : m.boundary_view(f) | std::views::enumerate)
			{
				auto corner_pos_span = std::span{ corner_pos_arr.data() + b.to_vertex_idx_offset, b.to_vertex_idx_count };

				for (auto&& [corner_idx, v] : m.vertex_view(b) | std::views::enumerate)
				{
					c_auto d					= m.position_vec[v.pos_idx] - origin;
					corner_pos_span[corner_idx] = { math::dot(d, f.u_basis), math::dot(d, f.v_basis) };
				}

				corner_span_ptr_arr[nth_boundary]	= corner_pos_span.data();
				corner_span_count_arr[nth_boundary] = static_cast<uint32>(corner_pos_span.size());
			}

			auto* p_idx_arr = (uint32*)nullptr;

			auto idx_size = detail::perform(
				(void**)corner_span_ptr_arr.data(),
				corner_span_count_arr.data(),
				f.to_boundary_idx_count,
				p_idx_arr);

			idx_vec.append_range(std::span{ p_idx_arr, idx_size }
								 | std::views::transform(
									 [offset, &m](auto idx) {
										 return m.boundary_to_vertex_idx_vec[idx + offset];
									 }));

			delete[] p_idx_arr;
		}

		return idx_vec;
	}
}	 // namespace age::external::earcut