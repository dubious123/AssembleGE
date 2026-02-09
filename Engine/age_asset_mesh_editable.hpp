#pragma once

namespace age::asset::e
{
	AGE_DEFINE_ENUM(corner_normal_weight_mode, uint8,
					edge,
					area,
					edge_area);
}

namespace age::asset
{
	struct mesh_editable
	{
		struct range
		{
			uint32 offset;
			uint32 count;
		};

		struct vertex_attribute
		{
			float3							  normal;	  // normalized
			float4							  tangent;	  // normalized
			std::array<float2, g::uv_set_max> uv_set;
			uint8							  uv_count;
		};

		struct vertex_adj
		{
			uint32 to_edge_idx_offset	  = 0;
			uint32 to_edge_idx_count	  = 0;
			uint32 to_boundary_idx_offset = 0;
			uint32 to_boundary_idx_count  = 0;
			uint32 to_face_idx_offset	  = 0;
			uint32 to_face_idx_count	  = 0;
		};

		struct vertex
		{
			uint32 pos_idx;
			uint32 attribute_idx;

			uint32 vertex_adj_idx;
		};

		struct edge
		{
			uint32 v0_idx;
			uint32 v1_idx;

			uint32 to_boundary_idx_offset;
			uint32 to_boundary_idx_count;
			uint32 to_face_idx_offset;
			uint32 to_face_idx_count;
		};

		struct boundary
		{
			uint32 to_vertex_idx_offset;
			uint32 to_vertex_idx_count;
			uint32 to_edge_idx_offset;
			uint32 to_face_idx_offset;
			uint32 to_face_idx_count;

			FORCE_INLINE constexpr uint32
			to_edge_idx_count() const
			{
				return to_vertex_idx_count;
			}
		};

		struct face
		{
			uint32 to_boundary_idx_offset;
			uint32 to_boundary_idx_count;

			// bake_flags bake_flag{ bake_flags::front_outer | bake_flags::back_outer | bake_flags::front_hole | bake_flags::back_hole };

		  public:
			constexpr FORCE_INLINE uint32
			to_outer_boundary_idx_offset() const noexcept
			{
				return to_boundary_idx_offset;
			}

			constexpr FORCE_INLINE uint32
			to_outer_boundary_idx_count() const noexcept
			{
				return 1;
			}

			constexpr FORCE_INLINE uint32
			to_hole_boundary_idx_offset() const noexcept
			{
				return to_boundary_idx_offset + 1;
			}

			constexpr FORCE_INLINE uint32
			to_hole_boundary_idx_count() const noexcept
			{
				return to_boundary_idx_count - 1;
			}
		};

		data_structure::vector<float3>			 position_vec{};
		data_structure::vector<vertex_attribute> vertex_attr_vec{};

		data_structure::vector<vertex_adj> vertex_adj_vec{};

		data_structure::vector<vertex>	 vertex_vec{};
		data_structure::vector<edge>	 edge_vec{};
		data_structure::vector<boundary> boundary_vec{};
		data_structure::vector<face>	 face_vec{};

		data_structure::vector<uint32> vertex_to_edge_idx_vec{};
		data_structure::vector<range>  vertex_to_edge_idx_free_range_vec{};

		data_structure::vector<uint32> vertex_to_boundary_idx_vec{};
		data_structure::vector<range>  vertex_to_boundary_free_range_vec{};

		data_structure::vector<uint32> vertex_to_face_idx_vec{};
		data_structure::vector<range>  vertex_to_face_free_range_vec{};

		data_structure::vector<uint32> edge_to_boundary_idx_vec{};
		data_structure::vector<range>  edge_to_boundary_idx_free_range_vec{};

		data_structure::vector<uint32> edge_to_face_idx_vec{};
		data_structure::vector<range>  edge_to_face_idx_free_range_vec{};

		data_structure::vector<uint32> boundary_to_vertex_idx_vec{};
		data_structure::vector<range>  boundary_to_vertex_free_range_vec{};

		data_structure::vector<uint32> boundary_to_edge_idx_vec{};
		data_structure::vector<range>  boundary_to_edge_free_range_vec{};

		data_structure::vector<uint32> boundary_to_face_idx_vec{};
		data_structure::vector<range>  boundary_to_face_free_range_vec{};

		data_structure::vector<uint32> face_to_boundary_idx_vec{};
		data_structure::vector<range>  face_to_boundary_idx_free_range_vec{};

		std::span<const uint32>
		vertex_idx_span(const boundary& b) const noexcept
		{
			return std::span<const uint32>{ boundary_to_vertex_idx_vec.data() + b.to_vertex_idx_offset, b.to_vertex_idx_count };
		}

		std::span<uint32>
		vertex_idx_span(boundary& b) noexcept
		{
			return std::span<uint32>{ boundary_to_vertex_idx_vec.data() + b.to_vertex_idx_offset, b.to_vertex_idx_count };
		}

		decltype(auto)
		vertex_view(const edge& e) const noexcept
		{
			return std::array{ e.v0_idx, e.v1_idx } | std::views::transform([this](auto idx) -> c_auto& { return vertex_vec[idx]; });
		}

		decltype(auto)
		vertex_view(edge& e) noexcept
		{
			return std::array{ e.v0_idx, e.v1_idx } | std::views::transform([this](auto idx) -> auto& { return vertex_vec[idx]; });
		}

		decltype(auto)
		vertex_view(const boundary& b) const noexcept
		{
			return vertex_idx_span(b) | std::views::transform([this](auto v_idx) -> c_auto& { return vertex_vec[v_idx]; });
		}

		decltype(auto)
		vertex_view(boundary& b) noexcept
		{
			return vertex_idx_span(b) | std::views::transform([this](auto v_idx) -> auto& { return vertex_vec[v_idx]; });
		}

		decltype(auto)
		vertex_view(const face& f) const noexcept
		{
			return boundary_view(f)
				 | std::views::transform([this](const boundary& b) noexcept -> decltype(auto) { return vertex_view(b); })
				 | std::views::join;
		}

		decltype(auto)
		vertex_view(face& f) noexcept
		{
			return boundary_view(f)
				 | std::views::transform([this](boundary& b) noexcept -> decltype(auto) { return vertex_view(b); })
				 | std::views::join;
		}

		std::span<const uint32>
		edge_idx_span(const vertex& v) const noexcept
		{
			const auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
			return std::span<const uint32>{ vertex_to_edge_idx_vec.data() + v_adj.to_edge_idx_offset, v_adj.to_edge_idx_count };
		}

		std::span<uint32>
		edge_idx_span(vertex& v) noexcept
		{
			const auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
			return std::span<uint32>{ vertex_to_edge_idx_vec.data() + v_adj.to_edge_idx_offset, v_adj.to_edge_idx_count };
		}

		std::span<const uint32>
		edge_idx_span(const boundary& b) const noexcept
		{
			return std::span<const uint32>{ boundary_to_edge_idx_vec.data() + b.to_edge_idx_offset, b.to_edge_idx_count() };
		}

		std::span<uint32>
		edge_idx_span(boundary& b) noexcept
		{
			return std::span<uint32>{ boundary_to_edge_idx_vec.data() + b.to_edge_idx_offset, b.to_edge_idx_count() };
		}

		decltype(auto)
		edge_view(c_auto& o) const noexcept
			requires requires { edge_idx_span(o); }
		{
			return edge_idx_span(o)
				 | std::views::transform([this](c_auto e_idx) -> c_auto& { return edge_vec[e_idx]; });
		}

		decltype(auto)
		edge_view(auto& o) noexcept
			requires requires { edge_idx_span(o); }
		{
			return edge_idx_span(o)
				 | std::views::transform([this](auto e_idx) -> auto& { return edge_vec[e_idx]; });
		}

		decltype(auto)
		hole_edge_view(const face& f) const noexcept
		{
			return hole_boundary_idx_span(f)
				 | std::views::transform([this](auto hole_boundary_idx) -> decltype(auto) { return edge_view(std::cref(boundary_vec[hole_boundary_idx])); })
				 | std::views::join;
		}

		decltype(auto)
		hole_edge_view(face& f) noexcept
		{
			return hole_boundary_idx_span(f)
				 | std::views::transform([this](auto hole_boundary_idx) -> decltype(auto) { return edge_view(boundary_vec[hole_boundary_idx]); })
				 | std::views::join;
		}

		decltype(auto)
		outer_edge_view(const face& f) const noexcept
		{
			return edge_view(std::cref(boundary_vec[face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]]));
		}

		decltype(auto)
		outer_edge_view(const face& f) noexcept
		{
			return edge_view(boundary_vec[face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]]);
		}

		decltype(auto)
		edge_view(const face& f) const noexcept
		{
			return boundary_view(f)
				 | std::views::transform([this](const boundary& b) noexcept -> decltype(auto) { return edge_view(b); })
				 | std::views::join;
		}

		decltype(auto)
		edge_view(const face& f) noexcept
		{
			return boundary_view(f)
				 | std::views::transform([this](boundary& b) noexcept -> decltype(auto) { return edge_view(b); })
				 | std::views::join;
		}

		std::span<const uint32>
		boundary_idx_span(const vertex& v) const noexcept
		{
			const auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
			return std::span<const uint32>{ &vertex_to_boundary_idx_vec[v_adj.to_boundary_idx_offset], v_adj.to_boundary_idx_count };
		}

		std::span<uint32>
		boundary_idx_span(const vertex& v) noexcept
		{
			const auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
			return std::span<uint32>{ &vertex_to_boundary_idx_vec[v_adj.to_boundary_idx_offset], v_adj.to_boundary_idx_count };
		}

		std::span<const uint32>
		boundary_idx_span(const edge& e) const noexcept
		{
			return std::span<const uint32>{ &edge_to_boundary_idx_vec[e.to_boundary_idx_offset], e.to_boundary_idx_count };
		}

		std::span<uint32>
		boundary_idx_span(const edge& e) noexcept
		{
			return std::span<uint32>{ &edge_to_boundary_idx_vec[e.to_boundary_idx_offset], e.to_boundary_idx_count };
		}

		std::span<const uint32>
		boundary_idx_span(const face& f) const noexcept
		{
			return std::span<const uint32>{ face_to_boundary_idx_vec.data() + f.to_boundary_idx_offset, f.to_boundary_idx_count };
		}

		std::span<uint32>
		boundary_idx_span(const face& f) noexcept
		{
			return std::span<uint32>{ face_to_boundary_idx_vec.data() + f.to_boundary_idx_offset, f.to_boundary_idx_count };
		}

		std::span<const uint32>
		hole_boundary_idx_span(const face& f) const noexcept
		{
			return std::span<const uint32>{ face_to_boundary_idx_vec.data() + f.to_hole_boundary_idx_offset(), f.to_hole_boundary_idx_count() };
		}

		std::span<uint32>
		hole_boundary_idx_span(const face& f) noexcept
		{
			return std::span<uint32>{ face_to_boundary_idx_vec.data() + f.to_hole_boundary_idx_offset(), f.to_hole_boundary_idx_count() };
		}

		decltype(auto)
		boundary_view(c_auto& o) const noexcept
			requires requires { boundary_idx_span(o); }
		{
			return boundary_idx_span(o)
				 | std::views::transform([this](auto b_idx) -> c_auto& { return boundary_vec[b_idx]; });
		}

		decltype(auto)
		boundary_view(c_auto& o) noexcept
			requires requires { boundary_idx_span(o); }
		{
			return boundary_idx_span(o)
				 | std::views::transform([this](auto b_idx) -> auto& { return boundary_vec[b_idx]; });
		}

		decltype(auto)
		hole_boundary_view(const face& f) const noexcept
		{
			return hole_boundary_idx_span(f)
				 | std::views::transform([this](auto b_idx) -> c_auto& { return boundary_vec[b_idx]; });
		}

		decltype(auto)
		hole_boundary_view(const face& f) noexcept
		{
			return hole_boundary_idx_span(f)
				 | std::views::transform([this](auto b_idx) -> auto& { return boundary_vec[b_idx]; });
		}

		FORCE_INLINE const boundary&
		outer_boundary(const face& f) const noexcept
		{
			return boundary_vec[face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]];
		}

		FORCE_INLINE boundary&
		outer_boundary(const face& f) noexcept
		{
			return boundary_vec[face_to_boundary_idx_vec[f.to_outer_boundary_idx_offset()]];
		}

		std::span<const uint32>
		face_idx_span(const vertex& v) const noexcept
		{
			const auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
			return std::span<const uint32>{ vertex_to_face_idx_vec.data() + v_adj.to_face_idx_offset, v_adj.to_face_idx_count };
		}

		std::span<uint32>
		face_idx_span(const vertex& v) noexcept
		{
			const auto& v_adj = vertex_adj_vec[v.vertex_adj_idx];
			return std::span<uint32>{ vertex_to_face_idx_vec.data() + v_adj.to_face_idx_offset, v_adj.to_face_idx_count };
		}

		std::span<const uint32>
		face_idx_span(const edge& e) const noexcept
		{
			return std::span<const uint32>{ edge_to_face_idx_vec.data() + e.to_face_idx_offset, e.to_face_idx_count };
		}

		std::span<uint32>
		face_idx_span(const edge& e) noexcept
		{
			return std::span<uint32>{ edge_to_face_idx_vec.data() + e.to_face_idx_offset, e.to_face_idx_count };
		}

		std::span<const uint32>
		face_idx_span(const boundary& b) const noexcept
		{
			return std::span<const uint32>{ boundary_to_face_idx_vec.data() + b.to_face_idx_offset, b.to_face_idx_count };
		}

		std::span<uint32>
		face_idx_span(const boundary& b) noexcept
		{
			return std::span<uint32>{ boundary_to_face_idx_vec.data() + b.to_face_idx_offset, b.to_face_idx_count };
		}

		decltype(auto)
		face_view(c_auto& o) const noexcept
			requires requires { face_idx_span(o); }
		{
			return face_idx_span(o) | std::views::transform([this](c_auto f_idx) -> c_auto& { return face_vec[f_idx]; });
		}

		decltype(auto)
		face_view(c_auto& o) noexcept
			requires requires { face_idx_span(o); }
		{
			return face_idx_span(o) | std::views::transform([this](c_auto f_idx) -> auto& { return face_vec[f_idx]; });
		}

		void
		clean_up() noexcept;

		void
		debug_validate() const noexcept;
	};
}	 // namespace age::asset
