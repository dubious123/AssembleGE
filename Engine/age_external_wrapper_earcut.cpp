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
			auto offset				   = m.outer_boundary(f).to_vertex_idx_offset;
			auto [proj_fptr, reversed] = [](const auto& m, const auto& f) {
				auto& b				= m.boundary_vec[m.face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]];
				auto  s				= m.vertex_idx_span(b);
				auto&& [v0, v1, v2] = std::tie(m.vertex_vec[s[0]], m.vertex_vec[s[1]], m.vertex_vec[s[2]]);
				auto&& [p0, p1, p2] = std::tie(m.position_vec[v0.pos_idx], m.position_vec[v1.pos_idx], m.position_vec[v2.pos_idx]);

				auto [x0, x1, x2] = simd::load(p0, p1, p2);

				auto snormal = simd::cross3(x2 - x0, x1 - x0);
				auto normal	 = snormal | simd::abs();
				auto mask	 = normal
							 | simd::max(simd::swizzle<1, 2, 0, 3>(normal))
							 | simd::max(simd::swizzle<2, 0, 1, 3>(normal))
							 | simd::cmp_equal(normal);

				if (simd::get_x<bool>(mask))
				{
					return std::pair{
						+[](const float3& p) { return std::pair{ p.y, p.z }; },
						simd::get_x(snormal) < 0.f
					};
				}
				else if (simd::get_y<bool>(mask))
				{
					return std::pair{
						+[](const float3& p) { return std::pair{ p.x, p.z }; },
						simd::get_y(snormal) < 0.f
					};
				}
				else
				{
					AGE_ASSERT(simd::get_z<bool>(mask));
					return std::pair{
						+[](const float3& p) { return std::pair{ p.x, p.y }; },
						simd::get_z(snormal) < 0.f
					};
				}
			}(m, f);

			auto corner_span_ptr_arr   = dynamic_array<std::pair<float, float>*>::gen_sized_default(f.to_boundary_idx_count);
			auto corner_span_count_arr = dynamic_array<uint32>::gen_sized_default(f.to_boundary_idx_count, corner_span_ptr_arr.data());

			for (auto&& [nth_boundary, b] : m.boundary_view(f) | std::views::enumerate)
			{
				auto corner_pos_span = std::span{ corner_pos_arr.data() + b.to_vertex_idx_offset, b.to_vertex_idx_count };

				for (auto&& [corner_idx, v] : m.vertex_view(b) | std::views::enumerate)
				{
					corner_pos_span[corner_idx] = proj_fptr(m.position_vec[v.pos_idx]);
				}

				if (reversed)
				{
					std::ranges::reverse(corner_pos_span);
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