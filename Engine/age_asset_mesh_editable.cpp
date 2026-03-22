#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset::detail
{
	FORCE_INLINE auto
	face_normal_cw(mesh_editable& mesh_edit, mesh_editable::face& f) noexcept
	{
		auto& b				   = mesh_edit.boundary_vec[mesh_edit.face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]];
		auto  s				   = mesh_edit.vertex_idx_span(b);
		auto&& [v0, v1, v2]	   = std::tie(mesh_edit.vertex_vec[s[0]], mesh_edit.vertex_vec[s[1]], mesh_edit.vertex_vec[s[2]]);
		auto&& [p0, p1, p2]	   = std::tie(mesh_edit.position_vec[v0.pos_idx], mesh_edit.position_vec[v1.pos_idx], mesh_edit.position_vec[v2.pos_idx]);
		auto&& [xm0, xm1, xm2] = simd::load(p0, p1, p2);
		return (xm1 - xm0) | simd::cross3(xm2 - xm1) | simd::normalize3();
	}

	void
	calculate_normal_by_angle(mesh_editable& mesh_edit) noexcept
	{
		for (auto&& [f_idx, f] : mesh_edit.face_vec | std::views::enumerate)
		{
			auto xm_f_normal = detail::face_normal_cw(mesh_edit, f);
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

					simd::mul_add(
						xm_f_normal,
						simd::angle_between_vec3(x0 - x1, x2 - x1),
						xm_v1_normal)
						| simd::store_to(mesh_edit.vertex_attr_vec[v1.attribute_idx].normal);
				}
			}
		}

		for (auto& adj : mesh_edit.vertex_attr_vec)
		{
			simd::load(adj.normal)
				| simd::normalize3()
				| simd::store_to(adj.normal);
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

		auto res = external::mikk::gen_tangent(mesh_edit);

		AGE_ASSERT(res);
	}
}	 // namespace age::asset

namespace age::asset
{
	mesh_editable
	create_primitive_mesh_plane(const primitive_desc& desc) noexcept
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

		c_auto u_basis = desc.local_basis[0];
		c_auto v_basis = -desc.local_basis[2];

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

		for (const auto&& [vertex_v_idx, vertex_u_idx] : std::views::cartesian_product(
				 std::views::iota(0u) | std::views::take(grid_v),
				 std::views::iota(0u) | std::views::take(grid_u)))
		{
			// UV layout:
			// (0,0)-----(1,0)
			//   |         |
			//   |         |
			// (0,1)-----(1,1)
			c_auto vertex_idx		   = vertex_v_idx * grid_u + vertex_u_idx;
			res.vertex_vec[vertex_idx] = {
				.pos_idx		= vertex_idx,
				.attribute_idx	= vertex_idx,
				.vertex_adj_idx = vertex_idx
			};

			c_auto u_offset = u_basis * (-0.5f * desc.size.x),
				   u_step	= u_basis * (desc.size.x / desc.seg_u),
				   v_offset = v_basis * (-0.5f * desc.size.z),
				   v_step	= v_basis * (desc.size.z / desc.seg_v);

			res.position_vec[vertex_idx] =
				desc.pos + (v_offset + v_step * vertex_v_idx) + (u_offset + u_step * vertex_u_idx);

			res.vertex_attr_vec[vertex_idx] = {
				.normal	  = desc.local_basis[1],
				.uv_set	  = { float2{ vertex_u_idx / static_cast<float>(desc.seg_u), vertex_v_idx / static_cast<float>(desc.seg_v) },
							  float2{ vertex_u_idx / static_cast<float>(desc.seg_u), vertex_v_idx / static_cast<float>(desc.seg_v) },
							  float2{ vertex_u_idx / static_cast<float>(desc.seg_u), vertex_v_idx / static_cast<float>(desc.seg_v) },
							  float2{ vertex_u_idx / static_cast<float>(desc.seg_u), vertex_v_idx / static_cast<float>(desc.seg_v) } },
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

			c_auto vertex_idx_arr = std::array{
				(face_v_idx + 0) * grid_u + (face_u_idx + 0),
				(face_v_idx + 0) * grid_u + (face_u_idx + 1),
				(face_v_idx + 1) * grid_u + (face_u_idx + 1),
				(face_v_idx + 1) * grid_u + (face_u_idx + 0)
			};

			// |-|-|-|  grid : 4, seg : 3  horizontal : vertical grid * seg_u + face_u
			c_auto edge_idx_arr = std::array{
				(face_v_idx + 0) * desc.seg_u + (face_u_idx + 0),						 // 0->1 (H)
				(face_v_idx + 0) * grid_u + (face_u_idx + 1) + edge_horizontal_count,	 // 1->2 (V)
				(face_v_idx + 1) * desc.seg_u + (face_u_idx + 0),						 // 3->2 (H)
				(face_v_idx + 0) * grid_u + (face_u_idx + 0) + edge_horizontal_count,	 // 0->3 (V)
			};

			auto& f = res.face_vec[face_idx];
			{
				f = {
					.to_boundary_idx_offset = boundary_idx,
					.to_boundary_idx_count	= 1,
					.u_basis				= u_basis,
					.v_basis				= v_basis
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

	mesh_editable
	create_primitive_mesh_cube(const primitive_desc& desc) noexcept
	{
		c_auto basis_r = desc.local_basis[0];
		c_auto basis_u = desc.local_basis[1];
		c_auto basis_f = desc.local_basis[2];

		c_auto size_r = desc.size.x;
		c_auto size_u = desc.size.y;
		c_auto size_f = desc.size.z;

		auto desc_r = age::asset::primitive_desc{
			.pos		 = basis_r * size_r * 0.5f,
			.size		 = float3{ size_f, size_r, size_u },
			.seg_u		 = desc.seg_u,
			.seg_v		 = desc.seg_v,
			.local_basis = float3x3{ basis_f, basis_r, basis_u },
		};
		auto desc_l = age::asset::primitive_desc{
			.pos		 = -basis_r * size_r * 0.5f,
			.size		 = float3{ size_f, size_r, size_u },
			.seg_u		 = desc.seg_u,
			.seg_v		 = desc.seg_v,
			.local_basis = float3x3{ -basis_f, -basis_r, basis_u },
		};
		auto desc_u = age::asset::primitive_desc{
			.pos		 = basis_u * size_u * 0.5f,
			.size		 = float3{ size_r, size_u, size_f },
			.seg_u		 = desc.seg_u,
			.seg_v		 = desc.seg_v,
			.local_basis = float3x3{ basis_r, basis_u, basis_f },
		};
		auto desc_d = age::asset::primitive_desc{
			.pos		 = -basis_u * size_u * 0.5f,
			.size		 = float3{ size_r, size_u, size_f },
			.seg_u		 = desc.seg_u,
			.seg_v		 = desc.seg_v,
			.local_basis = float3x3{ basis_r, -basis_u, -basis_f },
		};
		auto desc_f = age::asset::primitive_desc{
			.pos		 = -basis_f * size_f * 0.5f,
			.size		 = float3{ size_r, size_f, size_u },
			.seg_u		 = desc.seg_u,
			.seg_v		 = desc.seg_v,
			.local_basis = float3x3{ basis_r, -basis_f, basis_u },
		};
		auto desc_b = age::asset::primitive_desc{
			.pos		 = basis_f * size_f * 0.5f,
			.size		 = float3{ size_r, size_f, size_u },
			.seg_u		 = desc.seg_u,
			.seg_v		 = desc.seg_v,
			.local_basis = float3x3{ -basis_r, basis_f, basis_u },
		};

		c_auto res = merge(std::array{ create_primitive_mesh_plane(desc_r),
									   create_primitive_mesh_plane(desc_l),
									   create_primitive_mesh_plane(desc_u),
									   create_primitive_mesh_plane(desc_d),
									   create_primitive_mesh_plane(desc_f),
									   create_primitive_mesh_plane(desc_b) });


		if constexpr (age::config::debug_mode)
		{
			res.debug_validate();
		}

		return res;
	}

	mesh_editable
	create_primitive_mesh_cube_sphere(const primitive_desc& desc) noexcept
	{
		auto   res	  = create_primitive_mesh_cube(desc);
		c_auto radius = desc.size * 0.5f;

		for (auto&& [p, attr] : std::views::zip(res.position_vec, res.vertex_attr_vec))
		{
			attr.normal = normalize(p - desc.pos);

			p = desc.pos + attr.normal * radius;
		}

		// todo : reduce vertex

		return res;
	}
}	 // namespace age::asset

namespace age::asset
{
	mesh_editable
	create_primitive_mesh(const primitive_desc& desc) noexcept
	{
		switch (desc.mesh_kind)
		{
		case e::primitive_mesh_kind::plane:
		{
			return create_primitive_mesh_plane(desc);
		}
		case e::primitive_mesh_kind::cube:
		{
			return create_primitive_mesh_cube(desc);
		}
		// case e::primitive_mesh_kind::uv_sphere:
		//{
		//	return create_primitive_mesh_uv_sphere(desc);
		// }
		case e::primitive_mesh_kind::cube_sphere:
		{
			return create_primitive_mesh_cube_sphere(desc);
		}
		default:
			AGE_UNREACHABLE("invalid primitive mesh type {}", std::to_underlying(desc.mesh_kind));
			break;
		}
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

namespace age::asset
{
	mesh_editable
	merge(std::span<const mesh_editable> meshes) noexcept
	{
		mesh_editable result;

		// reserve phase
		{
			struct sizes
			{
				size_t position{};
				size_t vertex_attr{};
				size_t vertex_adj{};
				size_t vertex{};
				size_t edge{};
				size_t boundary{};
				size_t face{};
				size_t boundary_to_vertex{};
				size_t boundary_to_edge{};
				size_t boundary_to_face{};
				size_t vertex_to_edge{};
				size_t vertex_to_boundary{};
				size_t vertex_to_face{};
				size_t edge_to_boundary{};
				size_t edge_to_face{};
				size_t face_to_boundary{};
				size_t vertex_to_edge_free{};
				size_t vertex_to_boundary_free{};
				size_t vertex_to_face_free{};
				size_t edge_to_boundary_free{};
				size_t edge_to_face_free{};
				size_t boundary_to_vertex_free{};
				size_t boundary_to_edge_free{};
				size_t boundary_to_face_free{};
				size_t face_to_boundary_free{};
			} s;

			for (c_auto& m : meshes)
			{
				s.position				  += m.position_vec.size();
				s.vertex_attr			  += m.vertex_attr_vec.size();
				s.vertex_adj			  += m.vertex_adj_vec.size();
				s.vertex				  += m.vertex_vec.size();
				s.edge					  += m.edge_vec.size();
				s.boundary				  += m.boundary_vec.size();
				s.face					  += m.face_vec.size();
				s.boundary_to_vertex	  += m.boundary_to_vertex_idx_vec.size();
				s.boundary_to_edge		  += m.boundary_to_edge_idx_vec.size();
				s.boundary_to_face		  += m.boundary_to_face_idx_vec.size();
				s.vertex_to_edge		  += m.vertex_to_edge_idx_vec.size();
				s.vertex_to_boundary	  += m.vertex_to_boundary_idx_vec.size();
				s.vertex_to_face		  += m.vertex_to_face_idx_vec.size();
				s.edge_to_boundary		  += m.edge_to_boundary_idx_vec.size();
				s.edge_to_face			  += m.edge_to_face_idx_vec.size();
				s.face_to_boundary		  += m.face_to_boundary_idx_vec.size();
				s.vertex_to_edge_free	  += m.vertex_to_edge_idx_free_range_vec.size();
				s.vertex_to_boundary_free += m.vertex_to_boundary_free_range_vec.size();
				s.vertex_to_face_free	  += m.vertex_to_face_free_range_vec.size();
				s.edge_to_boundary_free	  += m.edge_to_boundary_idx_free_range_vec.size();
				s.edge_to_face_free		  += m.edge_to_face_idx_free_range_vec.size();
				s.boundary_to_vertex_free += m.boundary_to_vertex_free_range_vec.size();
				s.boundary_to_edge_free	  += m.boundary_to_edge_free_range_vec.size();
				s.boundary_to_face_free	  += m.boundary_to_face_free_range_vec.size();
				s.face_to_boundary_free	  += m.face_to_boundary_idx_free_range_vec.size();
			}

			result.position_vec.reserve(s.position);
			result.vertex_attr_vec.reserve(s.vertex_attr);
			result.vertex_adj_vec.reserve(s.vertex_adj);
			result.vertex_vec.reserve(s.vertex);
			result.edge_vec.reserve(s.edge);
			result.boundary_vec.reserve(s.boundary);
			result.face_vec.reserve(s.face);
			result.boundary_to_vertex_idx_vec.reserve(s.boundary_to_vertex);
			result.boundary_to_edge_idx_vec.reserve(s.boundary_to_edge);
			result.boundary_to_face_idx_vec.reserve(s.boundary_to_face);
			result.vertex_to_edge_idx_vec.reserve(s.vertex_to_edge);
			result.vertex_to_boundary_idx_vec.reserve(s.vertex_to_boundary);
			result.vertex_to_face_idx_vec.reserve(s.vertex_to_face);
			result.edge_to_boundary_idx_vec.reserve(s.edge_to_boundary);
			result.edge_to_face_idx_vec.reserve(s.edge_to_face);
			result.face_to_boundary_idx_vec.reserve(s.face_to_boundary);
			result.vertex_to_edge_idx_free_range_vec.reserve(s.vertex_to_edge_free);
			result.vertex_to_boundary_free_range_vec.reserve(s.vertex_to_boundary_free);
			result.vertex_to_face_free_range_vec.reserve(s.vertex_to_face_free);
			result.edge_to_boundary_idx_free_range_vec.reserve(s.edge_to_boundary_free);
			result.edge_to_face_idx_free_range_vec.reserve(s.edge_to_face_free);
			result.boundary_to_vertex_free_range_vec.reserve(s.boundary_to_vertex_free);
			result.boundary_to_edge_free_range_vec.reserve(s.boundary_to_edge_free);
			result.boundary_to_face_free_range_vec.reserve(s.boundary_to_face_free);
			result.face_to_boundary_idx_free_range_vec.reserve(s.face_to_boundary_free);
		}

		struct base
		{
			uint32 position;
			uint32 vertex_attr;
			uint32 vertex_adj;
			uint32 vertex;
			uint32 edge;
			uint32 boundary;
			uint32 face;
			uint32 boundary_to_vertex;
			uint32 boundary_to_edge;
			uint32 boundary_to_face;
			uint32 vertex_to_edge;
			uint32 vertex_to_boundary;
			uint32 vertex_to_face;
			uint32 edge_to_boundary;
			uint32 edge_to_face;
			uint32 face_to_boundary;
		};

		for (c_auto& m : meshes)
		{
			c_auto b = base{
				.position			= result.position_vec.size<uint32>(),
				.vertex_attr		= result.vertex_attr_vec.size<uint32>(),
				.vertex_adj			= result.vertex_adj_vec.size<uint32>(),
				.vertex				= result.vertex_vec.size<uint32>(),
				.edge				= result.edge_vec.size<uint32>(),
				.boundary			= result.boundary_vec.size<uint32>(),
				.face				= result.face_vec.size<uint32>(),
				.boundary_to_vertex = result.boundary_to_vertex_idx_vec.size<uint32>(),
				.boundary_to_edge	= result.boundary_to_edge_idx_vec.size<uint32>(),
				.boundary_to_face	= result.boundary_to_face_idx_vec.size<uint32>(),
				.vertex_to_edge		= result.vertex_to_edge_idx_vec.size<uint32>(),
				.vertex_to_boundary = result.vertex_to_boundary_idx_vec.size<uint32>(),
				.vertex_to_face		= result.vertex_to_face_idx_vec.size<uint32>(),
				.edge_to_boundary	= result.edge_to_boundary_idx_vec.size<uint32>(),
				.edge_to_face		= result.edge_to_face_idx_vec.size<uint32>(),
				.face_to_boundary	= result.face_to_boundary_idx_vec.size<uint32>(),
			};

			// raw data - no offset fixup
			result.position_vec.append_range(m.position_vec);
			result.vertex_attr_vec.append_range(m.vertex_attr_vec);

			// vertex_adj - offset indirection vec offsets
			for (auto adj : m.vertex_adj_vec)
			{
				adj.to_edge_idx_offset	   += b.vertex_to_edge;
				adj.to_boundary_idx_offset += b.vertex_to_boundary;
				adj.to_face_idx_offset	   += b.vertex_to_face;
				result.vertex_adj_vec.emplace_back(adj);
			}

			// vertex - offset pos, attr, adj indices
			for (auto v : m.vertex_vec)
			{
				v.pos_idx		 += b.position;
				v.attribute_idx	 += b.vertex_attr;
				v.vertex_adj_idx += b.vertex_adj;
				result.vertex_vec.emplace_back(v);
			}

			// edge - offset vertex indices + indirection offsets
			for (auto e : m.edge_vec)
			{
				e.v0_idx				 += b.vertex;
				e.v1_idx				 += b.vertex;
				e.to_boundary_idx_offset += b.edge_to_boundary;
				e.to_face_idx_offset	 += b.edge_to_face;
				result.edge_vec.emplace_back(e);
			}

			// boundary - offset indirection offsets
			for (auto bd : m.boundary_vec)
			{
				bd.to_vertex_idx_offset += b.boundary_to_vertex;
				bd.to_edge_idx_offset	+= b.boundary_to_edge;
				bd.to_face_idx_offset	+= b.boundary_to_face;
				result.boundary_vec.emplace_back(bd);
			}

			// face - offset boundary indirection offset
			for (auto f : m.face_vec)
			{
				f.to_boundary_idx_offset += b.face_to_boundary;
				result.face_vec.emplace_back(f);
			}

			// indirection vecs - values are indices into element vecs, offset accordingly
			for (auto idx : m.vertex_to_edge_idx_vec)
			{
				result.vertex_to_edge_idx_vec.emplace_back(idx + b.edge);
			}
			for (auto idx : m.vertex_to_boundary_idx_vec)
			{
				result.vertex_to_boundary_idx_vec.emplace_back(idx + b.boundary);
			}
			for (auto idx : m.vertex_to_face_idx_vec)
			{
				result.vertex_to_face_idx_vec.emplace_back(idx + b.face);
			}
			for (auto idx : m.edge_to_boundary_idx_vec)
			{
				result.edge_to_boundary_idx_vec.emplace_back(idx + b.boundary);
			}
			for (auto idx : m.edge_to_face_idx_vec)
			{
				result.edge_to_face_idx_vec.emplace_back(idx + b.face);
			}
			for (auto idx : m.boundary_to_vertex_idx_vec)
			{
				result.boundary_to_vertex_idx_vec.emplace_back(idx + b.vertex);
			}
			for (auto idx : m.boundary_to_edge_idx_vec)
			{
				result.boundary_to_edge_idx_vec.emplace_back(idx + b.edge);
			}
			for (auto idx : m.boundary_to_face_idx_vec)
			{
				result.boundary_to_face_idx_vec.emplace_back(idx + b.face);
			}
			for (auto idx : m.face_to_boundary_idx_vec)
			{
				result.face_to_boundary_idx_vec.emplace_back(idx + b.boundary);
			}

			// free range vecs - offset accordingly
			for (auto r : m.vertex_to_edge_idx_free_range_vec)
			{
				r.offset += b.vertex_to_edge;
				result.vertex_to_edge_idx_free_range_vec.emplace_back(r);
			}
			for (auto r : m.vertex_to_boundary_free_range_vec)
			{
				r.offset += b.vertex_to_boundary;
				result.vertex_to_boundary_free_range_vec.emplace_back(r);
			}
			for (auto r : m.vertex_to_face_free_range_vec)
			{
				r.offset += b.vertex_to_face;
				result.vertex_to_face_free_range_vec.emplace_back(r);
			}
			for (auto r : m.edge_to_boundary_idx_free_range_vec)
			{
				r.offset += b.edge_to_boundary;
				result.edge_to_boundary_idx_free_range_vec.emplace_back(r);
			}
			for (auto r : m.edge_to_face_idx_free_range_vec)
			{
				r.offset += b.edge_to_face;
				result.edge_to_face_idx_free_range_vec.emplace_back(r);
			}
			for (auto r : m.boundary_to_vertex_free_range_vec)
			{
				r.offset += b.boundary_to_vertex;
				result.boundary_to_vertex_free_range_vec.emplace_back(r);
			}
			for (auto r : m.boundary_to_edge_free_range_vec)
			{
				r.offset += b.boundary_to_edge;
				result.boundary_to_edge_free_range_vec.emplace_back(r);
			}
			for (auto r : m.boundary_to_face_free_range_vec)
			{
				r.offset += b.boundary_to_face;
				result.boundary_to_face_free_range_vec.emplace_back(r);
			}
			for (auto r : m.face_to_boundary_idx_free_range_vec)
			{
				r.offset += b.face_to_boundary;
				result.face_to_boundary_idx_free_range_vec.emplace_back(r);
			}
		}

		return result;
	}
}	 // namespace age::asset
