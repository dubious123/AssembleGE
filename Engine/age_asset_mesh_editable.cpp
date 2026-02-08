#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset::detail
{
	FORCE_INLINE auto
	face_normal(mesh_editable& mesh_edit, mesh_editable::face& f) noexcept
	{
		auto& b				= mesh_edit.boundary_vec[mesh_edit.face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]];
		auto  s				= mesh_edit.vertex_idx_span(b);
		auto&& [v0, v1, v2] = std::tie(mesh_edit.vertex_vec[s[0]], mesh_edit.vertex_vec[s[1]], mesh_edit.vertex_vec[s[2]]);
		auto&& [p0, p1, p2] = std::tie(mesh_edit.position_vec[v0.pos_idx], mesh_edit.position_vec[v1.pos_idx], mesh_edit.position_vec[v2.pos_idx]);

		auto [x0, x1, x2] = simd::load(p0, p1, p2);
		return simd::normalize3(simd::cross3(simd::sub(x2, x0), simd::sub(x1, x0)));
	}

	void
	calculate_normal_by_angle(mesh_editable& mesh_edit) noexcept
	{
		for (auto&& [f_idx, f] : mesh_edit.face_vec | std::views::enumerate)
		{
			auto xm_f_normal = detail::face_normal(mesh_edit, f);
			for (auto& b : mesh_edit.boundary_view(f))
			{
				for (const auto [v0, v1, v2] :
					 mesh_edit.vertex_idx_span(b)
						 | age::views::circular_adjacent<3>
						 | age::views::transform_each([&](auto v_idx) -> decltype(auto) {
							   return mesh_edit.vertex_vec[v_idx];
						   }))
				{
					auto&& [p0, p1, p2] = std::tie(mesh_edit.position_vec[v0.pos_idx], mesh_edit.position_vec[v1.pos_idx], mesh_edit.position_vec[v2.pos_idx]);

					auto [x0, x1, x2, xm_v1_normal] = simd::load(p0, p1, p2, mesh_edit.vertex_attr_vec[v1.attribute_idx].normal);
					simd::store(
						mesh_edit.vertex_attr_vec[v1.attribute_idx].normal,
						simd::mul_add(
							xm_f_normal,
							simd::angle_between_vec3(simd::sub(x0, x1), simd::sub(x2, x1)),
							xm_v1_normal));
				}
			}
		}

		for (auto& adj : mesh_edit.vertex_attr_vec)
		{
			simd::store(adj.normal, simd::normalize3(simd::load(adj.normal)));
		}
	}
}	 // namespace age::asset::detail

namespace age::asset
{
	void
	calculate_normal(mesh_editable& mesh_edit, const normal_calc_desc& desc) noexcept
	{
		switch (desc.calc_mode)
		{
		case normal_calc_desc::mode::angle:
		{
			detail::calculate_normal_by_angle(mesh_edit);
			return;
		}
		default:
		{
			AGE_UNREACHABLE("invalid normal calc mode : {}", normal_calc_desc::to_string(desc.calc_mode));
		}
		}
	}

	void
	calculate_tangent(mesh_editable& mesh_edit, const tangent_calc_desc& desc) noexcept
	{
		// no hole
		for (auto& f : mesh_edit.face_vec)
		{
			AGE_ASSERT(f.to_hole_boundary_idx_count() == 0);
		}

		// auto mikkt_space_interface = SMikkTSpaceInterface{
		//	.m_getNumFaces = [](const SMikkTSpaceContext* p_ctx) {
		//		auto res = static_cast<mesh_editable*>(p_ctx->m_pUserData)->face_vec.size();
		//		return static_cast<int>(res); },

		//	.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* p_ctx, const int f_idx) {
		//		auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
		//		return static_cast<int>( mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]).to_vertex_idx_count); },

		//	.m_getPosition	  = [](const SMikkTSpaceContext* p_ctx, float pos_out[], const int f_idx, const int outer_b2v_idx) {
		//		auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
		//		auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
		//		auto& p = mesh_edit.position_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].pos_idx];
		//		std::memcpy(pos_out, &p, sizeof(p)); },
		//	.m_getNormal	  = [](const SMikkTSpaceContext* p_ctx, float normal_out[], const int f_idx, const int outer_b2v_idx) {
		//		auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
		//		auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
		//		auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
		//		std::memcpy(normal_out, &v_attr.normal, sizeof(v_attr.normal)); },
		//	.m_getTexCoord	  = [](const SMikkTSpaceContext* p_ctx, float uv_out[], const int f_idx, const int outer_b2v_idx) {
		//		auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
		//		auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
		//		auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
		//		std::memcpy(uv_out, &v_attr.uv_set[0], sizeof(v_attr.uv_set[0])); },
		//	.m_setTSpaceBasic = [](const SMikkTSpaceContext* p_ctx, const float tangent_in[], const float fsign, const int f_idx, const int outer_b2v_idx) {
		//		auto& mesh_edit = *static_cast<mesh_editable*>(p_ctx->m_pUserData);
		//		auto& b = mesh_edit.outer_boundary(mesh_edit.face_vec[f_idx]);
		//		auto& v_attr = mesh_edit.vertex_attr_vec[ mesh_edit.vertex_vec[ mesh_edit.boundary_to_vertex_idx_vec[outer_b2v_idx]].attribute_idx];
		//
		//		v_attr.tangent = { tangent_in[0], tangent_in[1], tangent_in[2], fsign }; }
		//};

		// auto ctx = SMikkTSpaceContext{
		//	.m_pInterface = &mikkt_space_interface,
		//	.m_pUserData  = static_cast<void*>(&mesh_edit)
		// };

		// auto res = genTangSpaceDefault(&ctx);

		auto res = external::mikk::gen_tangent(mesh_edit);

		AGE_ASSERT(res);
	}
}	 // namespace age::asset

namespace age::asset
{
	mesh_editable
	create_primitive(const primitive_desc& desc) noexcept
	{
		AGE_ASSERT(age::math::simd::is_orthogonal_basis(desc.local_basis));
		AGE_ASSERT(desc.seg_u > 0);
		AGE_ASSERT(desc.seg_v > 0);
		AGE_ASSERT(desc.size[0] > 0.f);
		AGE_ASSERT(desc.size[2] > 0.f);

		c_auto grid_u				 = desc.seg_u + 1;
		c_auto grid_v				 = desc.seg_v + 1;
		c_auto vertex_count			 = grid_u * grid_v;
		c_auto edge_count			 = desc.seg_v * grid_u + desc.seg_u * grid_v;
		c_auto edge_horizontal_count = desc.seg_u * grid_v;
		c_auto boundary_count		 = desc.seg_u * desc.seg_v;
		c_auto face_count			 = boundary_count;

		// 1. generate_vertices_from_uv

		auto res = mesh_editable{};
		{
			res.position_vec.resize(vertex_count);
			res.vertex_attr_vec.resize(vertex_count);
			res.vertex_adj_vec.resize(vertex_count);
			res.vertex_vec.resize(vertex_count);
			res.edge_vec.resize(edge_count);
			res.boundary_vec.resize(boundary_count);
			res.face_vec.resize(face_count);
			// undirected edge, no self-loop, vertex_to_edge_count == edge_to_vertex_count == edge_count * 2
			res.vertex_to_edge_idx_vec.resize(edge_count * 2);
			res.vertex_to_boundary_idx_vec.resize(boundary_count * 4);
			res.vertex_to_face_idx_vec.resize(face_count * 4);
			res.edge_to_boundary_idx_vec.resize(boundary_count * 4);
			res.edge_to_face_idx_vec.resize(face_count * 4);
			res.boundary_to_vertex_idx_vec.resize(boundary_count * 4);
			res.boundary_to_edge_idx_vec.resize(boundary_count * 4);
			res.boundary_to_face_idx_vec.resize(boundary_count);
			res.face_to_boundary_idx_vec.resize(face_count);
		};

		auto v = std::vector{ vertex_count, mesh_editable{} };
		for (const auto&& [vertex_v_idx, vertex_u_idx] : std::views::cartesian_product(
				 std::views::iota(0u) | std::views::take(grid_v),
				 std::views::iota(0u) | std::views::take(grid_u)))
		{
			c_auto vertex_idx		   = vertex_v_idx * grid_u + vertex_u_idx;
			res.vertex_vec[vertex_idx] = {
				.pos_idx		= vertex_idx,
				.attribute_idx	= vertex_idx,
				.vertex_adj_idx = vertex_idx
			};

			c_auto u_offset = desc.local_basis[0] * (-0.5f * desc.size[0]),
				   u_step	= desc.local_basis[0] * (desc.size[0] / desc.seg_u),
				   v_offset = desc.local_basis[2] * (-0.5f * desc.size[1]),
				   v_step	= desc.local_basis[2] * (desc.size[1] / desc.seg_v);


			res.position_vec[vertex_idx] =
				(v_offset + v_step * vertex_v_idx) + (u_offset + u_step * vertex_u_idx);

			res.vertex_attr_vec[vertex_idx] = {
				.uv_set	  = { float2{ vertex_u_idx, vertex_v_idx }, float2{ vertex_u_idx, vertex_v_idx }, float2{ vertex_u_idx, vertex_v_idx }, float2{ vertex_u_idx, vertex_v_idx } },
				.uv_count = 4
			};

			auto& v_adj		 = res.vertex_adj_vec[vertex_idx];
			auto& v_adj_prev = res.vertex_adj_vec[std::max(static_cast<int32>(vertex_idx) - 1, 0)];

			v_adj.to_boundary_idx_offset = v_adj_prev.to_boundary_idx_offset + v_adj_prev.to_boundary_idx_count;
			v_adj.to_face_idx_offset	 = v_adj_prev.to_face_idx_offset + v_adj_prev.to_face_idx_count;
			v_adj.to_edge_idx_offset	 = v_adj_prev.to_edge_idx_offset + v_adj_prev.to_edge_idx_count;

			c_auto is_horizontal_boundary = vertex_u_idx % desc.seg_u == 0;
			c_auto is_vertical_boundary	  = vertex_v_idx % desc.seg_v == 0;
			c_auto to_face_idx_count	  = (is_vertical_boundary and is_horizontal_boundary) ? 1u
										  : (is_vertical_boundary or is_horizontal_boundary)  ? 2u
																							  : 4u;
			c_auto to_boundary_idx_count  = to_face_idx_count;

			v_adj.to_face_idx_count		= to_face_idx_count;
			v_adj.to_boundary_idx_count = to_boundary_idx_count;
			v_adj.to_edge_idx_count		= 4
										- uint32{ vertex_u_idx % desc.seg_u == 0 }
										- uint32{ vertex_v_idx % desc.seg_v == 0 };


			c_auto edge_right_idx = vertex_v_idx * desc.seg_u + vertex_u_idx;
			c_auto edge_up_idx	  = edge_horizontal_count + vertex_v_idx * grid_u + vertex_u_idx;

			if (vertex_u_idx != desc.seg_u)
			{
				c_auto e_idx   = edge_right_idx;
				auto&  e_right = res.edge_vec[edge_right_idx];
				e_right		   = {
					.v0_idx = vertex_idx,
					.v1_idx = vertex_idx + 1,
					//.to_boundary_idx_offset = e_prev.to_boundary_idx_offset + e_prev.to_boundary_idx_count,
					.to_boundary_idx_count = 2u - uint32{ is_vertical_boundary },
					//.to_face_idx_offset		= e_prev.to_face_idx_offset + e_prev.to_face_idx_count,
					.to_face_idx_count = 2u - uint32{ is_vertical_boundary }
				};
			}
			if (vertex_v_idx != desc.seg_v)
			{
				c_auto e_idx = edge_up_idx;
				auto&  e_up	 = res.edge_vec[edge_up_idx];
				e_up		 = {
					.v0_idx = vertex_idx,
					.v1_idx = vertex_idx + grid_u,
					//.to_boundary_idx_offset = e_prev.to_boundary_idx_offset + e_prev.to_boundary_idx_count,
					.to_boundary_idx_count = 2u - uint32{ is_horizontal_boundary },
					//.to_face_idx_offset		= e_prev.to_face_idx_offset + e_prev.to_face_idx_count,
					.to_face_idx_count = 2u - uint32{ is_horizontal_boundary }
				};
			}
		}

		for (auto& v_adj : res.vertex_adj_vec)
		{
			v_adj.to_edge_idx_count		= 0;
			v_adj.to_boundary_idx_count = 0;
			v_adj.to_face_idx_count		= 0;
		}

		// vertex_to_edge, v_adj
		for (auto edge_to_boundary_offset = 0u, edge_to_face_offset = 0u;
			 auto&& [e_idx, e] : res.edge_vec | std::views::enumerate)
		{
			auto& v0_adj = res.vertex_adj_vec[e.v0_idx];
			auto& v1_adj = res.vertex_adj_vec[e.v1_idx];

			res.vertex_to_edge_idx_vec[v0_adj.to_edge_idx_offset + v0_adj.to_edge_idx_count] = static_cast<uint32>(e_idx);
			res.vertex_to_edge_idx_vec[v1_adj.to_edge_idx_offset + v1_adj.to_edge_idx_count] = static_cast<uint32>(e_idx);

			++v0_adj.to_edge_idx_count;
			++v1_adj.to_edge_idx_count;

			e.to_boundary_idx_offset  = edge_to_boundary_offset;
			e.to_face_idx_offset	  = edge_to_face_offset;
			edge_to_boundary_offset	 += e.to_boundary_idx_count;
			edge_to_face_offset		 += e.to_face_idx_count;
		}

		for (auto& e : res.edge_vec)
		{
			e.to_boundary_idx_count = 0;
			e.to_face_idx_count		= 0;
		}


		for (const auto [face_v_idx, face_u_idx] : std::views::cartesian_product(
				 std::views::iota(0u) | std::views::take(desc.seg_v),
				 std::views::iota(0u) | std::views::take(desc.seg_u)))
		{
			c_auto face_idx		= face_v_idx * desc.seg_u + face_u_idx;
			c_auto boundary_idx = face_idx;

			c_auto vertex_uv_arr = std::array{
				uint32_2{ face_v_idx + 0, face_u_idx + 0 },
				uint32_2{ face_v_idx + 0, face_u_idx + 1 },
				uint32_2{ face_v_idx + 1, face_u_idx + 1 },
				uint32_2{ face_v_idx + 1, face_u_idx + 0 }
			};

			c_auto vertex_idx_arr = std::array{
				(face_v_idx + 0) * grid_u + (face_u_idx + 0),
				(face_v_idx + 0) * grid_u + (face_u_idx + 1),
				(face_v_idx + 1) * grid_u + (face_u_idx + 1),
				(face_v_idx + 1) * grid_u + (face_u_idx + 0)
			};

			c_auto edge_idx_arr = std::array{
				(face_v_idx + 0) * desc.seg_u + (face_u_idx + 0),
				(face_v_idx + 0) * grid_u + (face_u_idx + 1) + edge_horizontal_count,
				(face_v_idx + 1) * desc.seg_u + (face_u_idx + 0),
				(face_v_idx + 0) * grid_u + (face_u_idx + 0) + edge_horizontal_count
			};

			auto& f = res.face_vec[face_idx];
			{
				f = {
					.to_boundary_idx_offset = boundary_idx,
					.to_boundary_idx_count	= 1
				};
				age::util::assign_n(res.boundary_idx_span(f), boundary_idx);
			}

			auto& b = res.boundary_vec[boundary_idx];
			{
				b = {
					.to_vertex_idx_offset = boundary_idx * 4,
					.to_vertex_idx_count  = 4,
					.to_edge_idx_offset	  = boundary_idx * 4,
					.to_face_idx_offset	  = boundary_idx,
					.to_face_idx_count	  = 1,
				};

				age::util::assign_n(
					res.face_idx_span(b),
					face_idx);

				age::util::assign_n(
					res.vertex_idx_span(b),
					vertex_idx_arr[0],
					vertex_idx_arr[1],
					vertex_idx_arr[2],
					vertex_idx_arr[3]);

				age::util::assign_n(
					res.edge_idx_span(b),
					edge_idx_arr[0],
					edge_idx_arr[1],
					edge_idx_arr[2],
					edge_idx_arr[3]);

				// vertex_to_boundary, face
				for (c_auto v_idx : res.vertex_idx_span(b))
				{
					auto&  v_adj   = res.vertex_adj_vec[v_idx];
					c_auto v2b_idx = v_adj.to_boundary_idx_offset + v_adj.to_boundary_idx_count;
					c_auto v2f_idx = v2b_idx;

					res.vertex_to_boundary_idx_vec[v2b_idx] = boundary_idx;
					res.vertex_to_face_idx_vec[v2f_idx]		= face_idx;
					++v_adj.to_boundary_idx_count;
					++v_adj.to_face_idx_count;
				}

				// edge_to_boundary, face
				for (c_auto e_idx : res.edge_idx_span(b))
				{
					auto& e = res.edge_vec[e_idx];

					c_auto e2b_idx						  = e.to_boundary_idx_offset + e.to_boundary_idx_count;
					c_auto e2f_idx						  = e.to_face_idx_offset + e.to_face_idx_count;
					res.edge_to_boundary_idx_vec[e2b_idx] = boundary_idx;
					res.edge_to_face_idx_vec[e2f_idx]	  = face_idx;

					++e.to_boundary_idx_count;
					++e.to_face_idx_count;
				}
			}
		}

		calculate_normal(res,
						 normal_calc_desc{
							 .calc_mode = normal_calc_desc::mode::angle,
						 });
		calculate_tangent(res, tangent_calc_desc{});

		if constexpr (age::config::debug_mode)
		{
			res.debug_validate();
		}

		return res;
	}

	void
	mesh_editable::debug_validate() const noexcept
	{
		if constexpr (age::config::debug_mode)
		{
			c_auto v_n = static_cast<uint32>(vertex_vec.size());
			c_auto e_n = static_cast<uint32>(edge_vec.size());
			c_auto b_n = static_cast<uint32>(boundary_vec.size());
			c_auto f_n = static_cast<uint32>(face_vec.size());

			AGE_ASSERT(position_vec.size() == v_n);
			AGE_ASSERT(vertex_attr_vec.size() == v_n);
			AGE_ASSERT(vertex_adj_vec.size() == v_n);

			for (auto&& [v_idx, v] : vertex_vec | std::views::enumerate)
			{
				c_auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
				AGE_ASSERT(v.vertex_adj_idx == v_idx);
				AGE_ASSERT(v_adj.to_edge_idx_offset + v_adj.to_edge_idx_count <= vertex_to_edge_idx_vec.size());
				AGE_ASSERT(v_adj.to_boundary_idx_offset + v_adj.to_boundary_idx_count <= vertex_to_boundary_idx_vec.size());
				AGE_ASSERT(v_adj.to_face_idx_offset + v_adj.to_face_idx_count <= vertex_to_face_idx_vec.size());

				for (c_auto& e : edge_view(v))
				{
					AGE_ASSERT(e.v0_idx == v_idx or e.v1_idx == v_idx);
				}

				for (c_auto& b : boundary_view(v))
				{
					AGE_ASSERT(std::ranges::count(vertex_idx_span(b), v_idx) == 1);
				}

				for (c_auto& f : face_view(v))
				{
					AGE_ASSERT(
						std::ranges::count_if(vertex_view(f), [&](const vertex& _v) { return _v.vertex_adj_idx == v_idx; })
						== 1);
				}
			}

			for (auto&& [e_idx, e] : edge_vec | std::views::enumerate)
			{
				AGE_ASSERT(e.v0_idx != e.v1_idx);
				AGE_ASSERT(e.to_boundary_idx_offset + e.to_boundary_idx_count <= edge_to_boundary_idx_vec.size());
				AGE_ASSERT(e.to_face_idx_offset + e.to_face_idx_count <= edge_to_face_idx_vec.size());
				AGE_ASSERT(e.to_face_idx_count <= 2 and e.to_face_idx_count > 0);

				for (auto& v : vertex_view(e))
				{
					AGE_ASSERT(std::ranges::count(edge_idx_span(v), e_idx) == 1);
				}

				for (auto& b : boundary_view(e))
				{
					AGE_ASSERT(std::ranges::count(edge_idx_span(b), e_idx) == 1);
				}

				for (auto& f : face_view(e))
				{
					static_assert(std::is_reference_v<std::ranges::range_reference_t<decltype(edge_view(f))>>);

					AGE_ASSERT(
						std::ranges::count_if(edge_view(f), [&](const edge& _e) { return &_e == &e; })
						== 1);
				}
			}


			for (auto&& [b_idx, b] : boundary_vec | std::views::enumerate)
			{
				AGE_ASSERT(b.to_vertex_idx_count >= 3);
				AGE_ASSERT(b.to_vertex_idx_offset + b.to_vertex_idx_count <= boundary_to_vertex_idx_vec.size());
				AGE_ASSERT(b.to_edge_idx_offset + b.to_edge_idx_count() <= boundary_to_edge_idx_vec.size());
				AGE_ASSERT(b.to_face_idx_offset + b.to_face_idx_count <= boundary_to_face_idx_vec.size());

				for (auto& v : vertex_view(b))
				{
					AGE_ASSERT(std::ranges::count(boundary_idx_span(v), b_idx) == 1);
				}

				for (c_auto& [ e0, e1 ] : edge_view(b) | views::circular_adjacent<2>)
				{
					AGE_ASSERT(std::ranges::count(boundary_idx_span(e0), b_idx) == 1);

					AGE_ASSERT(uint32{ e0.v0_idx == e1.v0_idx }
								   + uint32{ e0.v0_idx == e1.v1_idx }
								   + uint32{ e0.v1_idx == e1.v1_idx }
								   + uint32{ e0.v1_idx == e1.v0_idx }
							   == 1);
				}

				for (c_auto& f : face_view(b))
				{
					AGE_ASSERT(std::ranges::count(boundary_idx_span(f), b_idx) == 1);
				}
			}

			for (auto&& [f_idx, f] : face_vec | std::views::enumerate)
			{
				AGE_ASSERT(f.to_boundary_idx_offset + f.to_boundary_idx_count <= face_to_boundary_idx_vec.size());

				for (auto& v : vertex_view(f))
				{
					AGE_ASSERT(std::ranges::count(face_idx_span(v), f_idx) == 1);
				}

				for (c_auto& e : edge_view(f))
				{
					AGE_ASSERT(std::ranges::count(face_idx_span(e), f_idx) == 1);
				}

				for (c_auto& b : boundary_view(f))
				{
					AGE_ASSERT(std::ranges::count(face_idx_span(b), f_idx) == 1);
				}
			}
		}
	}
}	 // namespace age::asset
