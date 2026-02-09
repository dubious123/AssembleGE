#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::earcut
{
	data_structure::vector<uint32>
	perform(const asset::mesh_editable& m) noexcept
	{
		auto idx_vec	= data_structure::vector<uint32>{};
		auto pos_2d_vec = data_structure::vector<std::pair<float, float>>{};
		pos_2d_vec.resize(m.boundary_to_vertex_idx_vec.size());

		for (auto& f : m.face_vec)
		{
			auto offset				   = m.outer_boundary(f).to_vertex_idx_offset;
			auto polygon			   = data_structure::vector<std::span<std::pair<float, float>>>{};
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


			polygon.resize(f.to_boundary_idx_count);

			for (auto&& [b_idx, b] : m.boundary_view(f) | std::views::enumerate)
			{
				polygon[b_idx] = { pos_2d_vec.data() + b.to_vertex_idx_offset, b.to_vertex_idx_count };

				for (auto&& [v_idx, v] : m.vertex_view(b) | std::views::enumerate)
				{
					c_auto& p			  = m.position_vec[v.pos_idx];
					polygon[b_idx][v_idx] = proj_fptr(p);
				}

				if (reversed)
				{
					std::ranges::reverse(polygon[b_idx]);
				}
			}

			idx_vec.append_range(mapbox::earcut(polygon)
								 | std::views::transform(
									 [offset](auto idx) {
										 return idx + offset;
									 }));
		}

		return idx_vec;
	}
}	 // namespace age::external::earcut