#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	template <>
	bool
	validate_header<e::kind::mesh_baked>(const file_header& header) noexcept
	{
		auto res = true;
		{
			c_auto tmp = header.asset_kind == e::kind::mesh_baked;
			AGE_ASSERT(tmp);
			res &= tmp;
		}
		{
			c_auto tmp = header.blob_alignment_log2 > 0
					 and header.blob_alignment_log2 == static_cast<uint8>(std::countr_zero(alignof(entry<e::kind::mesh_baked>::header)));
			AGE_ASSERT(tmp);
			res &= tmp;
		}

		return res;
	}

	std::array<char, config::max_asset_path_len>&
	entry<e::kind::mesh_baked>::get_path() const noexcept
	{
		return g::path_vec[path_id];
	}

	bool
	entry<e::kind::mesh_baked>::is_cpu_loaded() const noexcept
	{
		return p_blob != nullptr;
	}

	bool
	entry<e::kind::mesh_baked>::is_gpu_loaded() const noexcept
	{
		return AGE_IS_INVALID_ID(render_id) is_false;
	}

	const entry<e::kind::mesh_baked>::header&
	entry<e::kind::mesh_baked>::get_header() const noexcept
	{
		return *reinterpret_cast<const header*>(p_blob);
	}

	const mesh_baked_header&
	entry<e::kind::mesh_baked>::get_mesh_header() const noexcept
	{
		return *reinterpret_cast<const mesh_baked_header*>(meshlet_buffer_data());
	}

	const void*
	entry<e::kind::mesh_baked>::meshlet_buffer_data() const noexcept
	{
		return p_blob + sizeof(header);
	}

	uint64
	entry<e::kind::mesh_baked>::index_buffer_byte_offset() const noexcept
	{
		return get_header().meshlet_buffer_byte_size + sizeof(header);
	}

	uint64
	entry<e::kind::mesh_baked>::pos_buffer_byte_offset() const noexcept
	{
		return index_buffer_byte_offset() + get_header().index_count * sizeof(uint32);
	}

	const void*
	entry<e::kind::mesh_baked>::index_buffer_data() const noexcept
	{
		return p_blob + index_buffer_byte_offset();
	}

	const void*
	entry<e::kind::mesh_baked>::pos_buffer_data() const noexcept
	{
		return p_blob + pos_buffer_byte_offset();
	}
}	 // namespace age::asset

namespace age::asset::mesh_baked
{
	void
	cpu_unload(handle h_mesh) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();
		AGE_ASSERT(entry.is_cpu_loaded());

		using t_entry  = BARE_OF(entry);
		auto allocator = t_entry::allocator_type(alignof(t_entry::header));
		allocator.deallocate(entry.p_blob);
		entry.p_blob = nullptr;

		AGE_ASSERT(entry.is_cpu_loaded() is_false);
	}

	void
	cpu_load(handle h_mesh, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

		if (entry.is_cpu_loaded())
		{
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob = buf.release();
			return;
		}

		detail::build_mesh_baked(entry.get_path(), desc, v_kind);

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob = buf.release();
			return;
		}
		else
		{
			AGE_ASSERT(false);
		}
	}

	handle
	cpu_load(std::string_view mesh_name, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		c_auto h_mesh = asset::detail::load_common<e::kind::mesh_baked>(mesh_name);

		cpu_load(h_mesh, desc, v_kind);

		return h_mesh;
	}

	void
	cpu_load(handle h_mesh) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

		if (entry.is_cpu_loaded())
		{
			return;
		}

		if (auto buf = asset::read_asset_file(entry.get_path());
			buf.empty() is_false)
		{
			entry.p_blob = buf.release();
			return;
		}
	}

	handle
	cpu_load(std::string_view mesh_name) noexcept
	{
		c_auto h_mesh = asset::detail::load_common<e::kind::mesh_baked>(mesh_name);

		cpu_load(h_mesh);

		return h_mesh;
	}

	void
	add_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e::kind::mesh_baked>();
		AGE_ASSERT(entry.ref_counter < std::numeric_limits<BARE_OF(entry.ref_counter)>::max());
		++entry.ref_counter;
	}

	void
	remove_ref(handle h) noexcept
	{
		auto& entry = h.get_entry<e::kind::mesh_baked>();
		AGE_ASSERT(entry.ref_counter > 0);
		--entry.ref_counter;
	}
}	 // namespace age::asset::mesh_baked

namespace age::asset::mesh_baked::detail
{
	void
	build_mesh_baked(const std::array<char, config::max_asset_path_len>& mesh_path, const primitive_desc& desc, e::vertex_kind e_kind) noexcept
	{
		auto asset_header = entry<e::kind::mesh_baked>::header{};
		auto mesh_header  = mesh_baked_header{};

		c_auto& mesh_edit = create_primitive_mesh(desc);

		auto mesh_fat = asset::triangulate<vertex_fat>(mesh_edit);
		AGE_ASSERT(mesh_fat.v_idx_vec.size() % 3 == 0);
		for (auto [nth, idx] : mesh_fat.v_idx_vec | std::views::enumerate)
		{
			AGE_ASSERT(idx < mesh_fat.vertex_vec.size());
		}

		auto&& [index_buffer, vertex_buffer] = external::meshopt::gen_remap(mesh_fat.v_idx_vec, mesh_fat.vertex_vec);

		external::meshopt::opt_reorder_buffers(index_buffer, vertex_buffer);

		auto&& [meshlet_global_index_buffer, meshlet_local_index_buffer, meshlet_header_vec, meshlet_vec] =
			external::meshopt::gen_meshlets(
				index_buffer,
				vertex_buffer);


		auto aabb_min = float3{ std::numeric_limits<float>::max() };
		auto aabb_max = float3{ std::numeric_limits<float>::lowest() };
		for (c_auto& v : vertex_buffer)
		{
			aabb_min = age::min(aabb_min, v.pos);
			aabb_max = age::max(aabb_max, v.pos);
		}

		c_auto aabb_size = age::max(aabb_max - aabb_min, float3{ age::g::epsilon_1e6 });

		auto buf = byte_buf::gen_reserved(
			sizeof(asset_header)
			+ sizeof(mesh_header)
			+ e::visit(e_kind, AGE_LAMBDA(<e::vertex_kind e_kind>(), { return static_cast<uint32>(sizeof(t_vertex_kind<e_kind>)); })) * vertex_buffer.size<uint32>()
			+ meshlet_header_vec.byte_size()
			+ meshlet_vec.byte_size()
			+ meshlet_global_index_buffer.byte_size()
			+ meshlet_local_index_buffer.byte_size()
			+ meshlet_local_index_buffer.size() * sizeof(uint32)	// flat index buffer
			+ vertex_buffer.size() * sizeof(float3)					// pos buffer
		);

		buf.move_write_pos(sizeof(asset_header) + sizeof(mesh_header));

		{
			c_auto base = static_cast<uint32>(sizeof(asset_header));

			AGE_ASSERT((buf.size<uint32>() - base) % 4 == 0);	 // vertex_quantized_buffer_offset
			std::ranges::for_each(vertex_buffer, [aabb_min, aabb_size, e_kind, &buf](c_auto& v) {
				e::visit(e_kind, AGE_LAMBDA(<e::vertex_kind e_kind>(auto& buf, auto&&... arg), { return buf.write(cvt_vertex_to<e_kind>(FWD(arg)...)); }), buf, v, aabb_min, aabb_size);
			});

			mesh_header.meshlet_header_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.meshlet_header_buffer_offset % 4 == 0);
			buf.write_bytes(meshlet_header_vec.data(), meshlet_header_vec.byte_size());

			mesh_header.meshlet_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.meshlet_buffer_offset % 4 == 0);
			buf.write_bytes(meshlet_vec.data(), meshlet_vec.byte_size());

			mesh_header.global_vertex_index_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.global_vertex_index_buffer_offset % 4 == 0);
			buf.write_bytes(meshlet_global_index_buffer.data(), meshlet_global_index_buffer.byte_size());

			mesh_header.local_vertex_index_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.local_vertex_index_buffer_offset % 4 == 0);
			buf.write_bytes(meshlet_local_index_buffer.data(), meshlet_local_index_buffer.byte_size());

			mesh_header.meshlet_count		  = meshlet_vec.size<uint32>();
			mesh_header.aabb_min			  = aabb_min;
			mesh_header.aabb_size			  = aabb_size;
			mesh_header.vertex_kind_and_extra = to_idx(e_kind);

			asset_header.meshlet_buffer_byte_size = buf.size() - base;
			asset_header.index_count			  = meshlet_local_index_buffer.size<uint32>();
			asset_header.pos_count				  = vertex_buffer.size<uint32>();

			{
				c_auto idx_buffer_offset = buf.size();
				for (c_auto& mshlt : meshlet_vec)
				{
					for (c_auto i : std::views::iota(mshlt.primitive_offset) | std::views::take(mshlt.primitive_count * 3))
					{
						buf.write_at(idx_buffer_offset + i * sizeof(uint32), meshlet_global_index_buffer[mshlt.global_index_offset + meshlet_local_index_buffer[i]]);
					}
				}

				buf.move_write_pos(idx_buffer_offset + sizeof(uint32) * meshlet_local_index_buffer.size());

				for (c_auto& v : vertex_buffer)
				{
					buf.write(v.pos);
				}
			}
		}

		AGE_ASSERT(buf.capacity() == buf.size());

		buf.write_at(0, asset_header, mesh_header);
		c_auto f_header = get_default_file_header<e::kind::mesh_baked>(buf.size(), static_cast<uint8>(std::countr_zero(alignof(decltype(asset_header)))));
		write_asset_file(mesh_path.data(), f_header, buf.data());
		return;
	}
}	 // namespace age::asset::mesh_baked::detail
