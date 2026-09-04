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

	age::array<char, config::max_asset_path_len>&
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

	namespace detail
	{
		FORCE_INLINE const std::byte*
		get_submesh_soa_ptr(const entry<e::kind::mesh_baked>& e) noexcept
		{
			return e.p_blob + sizeof(entry<e::kind::mesh_baked>::header) + e.get_mesh_header().submesh_data_offset;
		}
	}	 // namespace detail

	uint32
	entry<e::kind::mesh_baked>::submesh_count() const noexcept
	{
		return get_mesh_header().submesh_count;
	}

	uint32
	entry<e::kind::mesh_baked>::submesh_meshlet_id_offset(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());
		return reinterpret_cast<const uint32*>(detail::get_submesh_soa_ptr(*this))[submesh_id];
	}

	uint32
	entry<e::kind::mesh_baked>::submesh_meshlet_count(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());

		c_auto offset_next = submesh_id == submesh_count() - 1
							   ? get_mesh_header().meshlet_count
							   : submesh_meshlet_id_offset(submesh_id + 1);

		return offset_next - submesh_meshlet_id_offset(submesh_id);
	}

	uint32
	entry<e::kind::mesh_baked>::submesh_primitive_id_offset(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());
		return reinterpret_cast<const uint32*>(detail::get_submesh_soa_ptr(*this))[submesh_count() + submesh_id];
	}

	uint32
	entry<e::kind::mesh_baked>::submesh_primitive_count(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());

		c_auto offset_next = submesh_id == submesh_count() - 1
							   ? get_header().index_count / 3
							   : submesh_primitive_id_offset(submesh_id + 1);

		return offset_next - submesh_primitive_id_offset(submesh_id);
	}

	graphics::e::mesh_raster_mode_kind
	entry<e::kind::mesh_baked>::submesh_raster_mode(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());
		c_auto* p_mode = reinterpret_cast<const uint16*>(detail::get_submesh_soa_ptr(*this) + sizeof(uint32) * 2u * submesh_count());
		return static_cast<graphics::e::mesh_raster_mode_kind>(p_mode[submesh_id] & 0xffu);
	}

	graphics::e::mesh_rt_alpha_test_mode_kind
	entry<e::kind::mesh_baked>::submesh_rt_alpha_test_mode(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());
		c_auto* p_mode = reinterpret_cast<const uint16*>(detail::get_submesh_soa_ptr(*this) + sizeof(uint32) * 2u * submesh_count());
		return static_cast<graphics::e::mesh_rt_alpha_test_mode_kind>(p_mode[submesh_id] >> 8u);
	}

	graphics::e::mesh_rt_bake_mode_kind
	entry<e::kind::mesh_baked>::submesh_rt_bake_mode(uint32 submesh_id) const noexcept
	{
		AGE_ASSERT(submesh_id < submesh_count());
		c_auto* p_bake = p_blob + pos_buffer_byte_offset() + get_header().pos_count * sizeof(float3);
		return static_cast<graphics::e::mesh_rt_bake_mode_kind>(p_bake[submesh_id]);
	}

	bool
	entry<e::kind::mesh_baked>::allow_disable_omm() const noexcept
	{
		return ((get_mesh_header().vertex_kind_and_extra >> 8u) & 1u) != 0u;
	}
}	 // namespace age::asset

namespace age::asset::mesh_baked
{
	namespace detail
	{
		bool
		cpu_load_helper(entry<e::kind::mesh_baked>& entry) noexcept
		{
			if (auto file_data = asset::read_asset_file(entry.get_path());
				file_data.is_valid())
			{
				switch (file_data.header.asset_version)
				{
				case 0u:
				{
					auto& buf						   = file_data.buf;
					auto  asset_header				   = buf.read<asset::entry<e::kind::mesh_baked>::header>();
					auto  mesh_header				   = buf.read<mesh_baked_header>();
					mesh_header.vertex_kind_and_extra &= 0x0000'00ff;	 // do not allow disable omm
					mesh_header.submesh_count		   = 1u;
					mesh_header.submesh_data_offset	   = cast_to<uint32>(asset_header.meshlet_buffer_byte_size);
					c_auto size_before				   = buf.size();
					buf.resize(buf.size() + sizeof(uint32_3 /*meshlet_id_offset, primitive_id_offset, 2 runtime flags, 1 bake flags*/) + sizeof(graphics::e::mesh_rt_bake_mode_kind), buf.data());

					std::memmove(buf.data() + sizeof(asset_header) + asset_header.meshlet_buffer_byte_size + sizeof(uint32_3),
								 buf.data() + sizeof(asset_header) + asset_header.meshlet_buffer_byte_size,
								 size_before - sizeof(asset_header) - asset_header.meshlet_buffer_byte_size);

					c_auto submesh_soa = uint32_3{ 0, 0, uint32{ (to_idx<uint32>(graphics::e::mesh_raster_mode_kind::opaque) << 0u) | (to_idx<uint32>(graphics::e::mesh_rt_alpha_test_mode_kind::opaque) << 8u) } };
					c_auto bake_flags  = to_idx(graphics::e::mesh_rt_bake_mode_kind::opaque);

					std::memcpy(buf.data() + asset_header.meshlet_buffer_byte_size + sizeof(asset_header), &submesh_soa, sizeof(submesh_soa));
					asset_header.meshlet_buffer_byte_size += sizeof(submesh_soa);

					std::memcpy(buf.data(), &asset_header, sizeof(asset_header));
					std::memcpy(buf.data() + sizeof(asset_header), &mesh_header, sizeof(mesh_header));
					std::memcpy(buf.data() + size_before + sizeof(uint32_3), &bake_flags, sizeof(bake_flags));

					entry.p_blob = buf.release();

					entry.aabb_min = mesh_header.aabb_min;
					entry.aabb_max = mesh_header.aabb_min + mesh_header.aabb_size;
					return true;
				}
				case config::mesh_baked_asset_version:
				{
					entry.p_blob		= file_data.buf.release();
					c_auto& mesh_header = entry.get_mesh_header();
					entry.aabb_min		= mesh_header.aabb_min;
					entry.aabb_max		= mesh_header.aabb_min + mesh_header.aabb_size;
					return true;
				}
				default:
				{
					AGE_ASSERT(false);
					return false;
				}
				}

				return true;
			}

			return false;
		}
	}	 // namespace detail

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
	cpu_load(handle h_mesh, std::span<const primitive_desc> descs, e::vertex_kind v_kind) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

		if (entry.is_cpu_loaded())
		{
			return;
		}

		if (detail::cpu_load_helper(entry))
		{
			return;
		}

		detail::build_mesh_baked(entry.get_path(), descs, v_kind);

		if (detail::cpu_load_helper(entry))
		{
			return;
		}

		AGE_ASSERT(false);
	}

	void
	cpu_load(handle h_mesh, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		cpu_load(h_mesh, { &desc, 1 }, v_kind);
	}

	handle
	cpu_load(std::string_view mesh_name, std::span<const primitive_desc> descs, e::vertex_kind v_kind) noexcept
	{
		c_auto h_mesh = asset::detail::load_common<e::kind::mesh_baked>(mesh_name);

		cpu_load(h_mesh, descs, v_kind);

		return h_mesh;
	}

	handle
	cpu_load(std::string_view mesh_name, const primitive_desc& desc, e::vertex_kind v_kind) noexcept
	{
		return cpu_load(mesh_name, { &desc, 1 }, v_kind);
	}

	void
	cpu_load(handle h_mesh) noexcept
	{
		auto& entry = h_mesh.get_entry<e::kind::mesh_baked>();

		if (entry.is_cpu_loaded())
		{
			return;
		}

		if (detail::cpu_load_helper(entry))
		{
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
}	 // namespace age::asset::mesh_baked

namespace age::asset::mesh_baked::detail
{
	void
	build_mesh_baked(const age::array<char, config::max_asset_path_len>& mesh_path, std::span<const primitive_desc> descs, e::vertex_kind e_kind) noexcept
	{
		AGE_ASSERT(descs.empty() is_false);

		auto asset_header = entry<e::kind::mesh_baked>::header{};
		auto mesh_header  = mesh_baked_header{};

		auto vertex_buffer_arr				 = age::dynamic_array<age::vector<vertex_fat>>::gen_sized_default(descs.size());
		auto meshlet_global_index_buffer_arr = age::dynamic_array<age::vector<uint32>>::gen_sized_default(descs.size());
		auto meshlet_local_index_buffer_arr	 = age::dynamic_array<age::vector<uint8>>::gen_sized_default(descs.size());
		auto meshlet_header_buffer_arr		 = age::dynamic_array<age::vector<meshlet_header>>::gen_sized_default(descs.size());
		auto meshlet_buffer_arr				 = age::dynamic_array<age::vector<meshlet>>::gen_sized_default(descs.size());

		for (auto&& [idx, desc] : descs | views::enumerate<uint32>)
		{
			// not supported yet
			AGE_ASSERT(desc.rt_bake_mode != graphics::e::mesh_rt_bake_mode_kind::omm_opaque and desc.rt_bake_mode != graphics::e::mesh_rt_bake_mode_kind::omm_transparent);

			c_auto& mesh_edit = create_primitive_mesh(desc);

			auto mesh_fat = asset::triangulate<vertex_fat>(mesh_edit);
			AGE_ASSERT(mesh_fat.v_idx_vec.size() % 3 == 0);
			for (auto [nth, idx] : mesh_fat.v_idx_vec | std::views::enumerate)
			{
				AGE_ASSERT(idx < mesh_fat.vertex_vec.size());
			}

			auto&& [index_buffer, vertex_buffer] = external::meshopt::gen_remap(mesh_fat.v_idx_vec, mesh_fat.vertex_vec);
			external::meshopt::opt_reorder_buffers(index_buffer, vertex_buffer);

			auto&& [meshlet_global_index_buffer, meshlet_local_index_buffer, meshlet_header_buffer, meshlet_buffer] =
				external::meshopt::gen_meshlets(
					index_buffer,
					vertex_buffer);

			vertex_buffer_arr[idx]				 = std::move(vertex_buffer);
			meshlet_global_index_buffer_arr[idx] = std::move(meshlet_global_index_buffer);
			meshlet_local_index_buffer_arr[idx]	 = std::move(meshlet_local_index_buffer);
			meshlet_header_buffer_arr[idx]		 = std::move(meshlet_header_buffer);
			meshlet_buffer_arr[idx]				 = std::move(meshlet_buffer);
		}

		auto aabb_min						 = float3{ std::numeric_limits<float>::max() };
		auto aabb_max						 = float3{ std::numeric_limits<float>::lowest() };
		auto vertex_count					 = 0u;
		auto global_index_buffer_size		 = 0u;
		auto local_index_buffer_size		 = 0u;	  // == flat index entry count
		auto meshlet_header_buffer_byte_size = 0u;
		auto meshlet_count					 = 0u;
		auto meshlet_buffer_byte_size		 = 0u;

		for (auto&& [vertex_buffer, meshlet_global_index_buffer, meshlet_local_index_buffer, meshlet_header_buffer, meshlet_buffer] :
			 std::views::zip(vertex_buffer_arr, meshlet_global_index_buffer_arr, meshlet_local_index_buffer_arr, meshlet_header_buffer_arr, meshlet_buffer_arr))
		{
			for (c_auto& v : vertex_buffer)
			{
				aabb_min = min(aabb_min, v.pos);
				aabb_max = max(aabb_max, v.pos);
			}
			vertex_count					+= vertex_buffer.size<uint32>();
			global_index_buffer_size		+= meshlet_global_index_buffer.size<uint32>();
			local_index_buffer_size			+= meshlet_local_index_buffer.size<uint32>();
			meshlet_header_buffer_byte_size += meshlet_header_buffer.byte_size<uint32>();
			meshlet_count					+= meshlet_buffer.size<uint32>();
			meshlet_buffer_byte_size		+= meshlet_buffer.byte_size<uint32>();
		}

		c_auto aabb_size	 = age::max(aabb_max - aabb_min, float3{ age::g::epsilon_1e6 });
		c_auto submesh_count = static_cast<uint32>(descs.size());
		c_auto vertex_stride = e::visit(e_kind, AGE_LAMBDA(<e::vertex_kind e_kind>(), { return static_cast<uint32>(sizeof(t_vertex_kind<e_kind>)); }));

		static_assert(sizeof(uint16) == sizeof(graphics::e::mesh_raster_mode_kind) + sizeof(graphics::e::mesh_rt_alpha_test_mode_kind));

		auto buf = byte_buf::gen_reserved(
			sizeof(asset_header)
			+ sizeof(mesh_header)
			+ vertex_stride * vertex_count
			+ meshlet_header_buffer_byte_size
			+ meshlet_buffer_byte_size
			+ global_index_buffer_size * sizeof(uint32)
			+ util::align_up(local_index_buffer_size * sizeof(uint8), 4u)
			+ submesh_count * sizeof(uint32_2)								 // submesh meshlet_id_offset, primitive_id_offset
			+ util::align_up(submesh_count * sizeof(uint16), 4u)			 // submesh flags
			+ local_index_buffer_size * sizeof(uint32)						 // flat index buffer
			+ vertex_count * sizeof(float3)									 // pos buffer
			+ submesh_count * sizeof(graphics::e::mesh_rt_bake_mode_kind)	 // rt_bake_mode tail
		);

		buf.move_write_pos(sizeof(asset_header) + sizeof(mesh_header));

		{
			c_auto base = static_cast<uint32>(sizeof(asset_header));

			// vertex
			AGE_ASSERT((buf.size<uint32>() - base) % 4 == 0);	 // vertex_quantized_buffer_offset
			for (c_auto& vertex_buffer : vertex_buffer_arr)
			{
				std::ranges::for_each(vertex_buffer, [aabb_min, aabb_size, e_kind, &buf](c_auto& v) {
					e::visit(e_kind, AGE_LAMBDA(<e::vertex_kind e_kind>(auto& buf, auto&&... arg), { return buf.write(cvt_vertex_to<e_kind>(FWD(arg)...)); }), buf, v, aabb_min, aabb_size);
				});
			}

			// meshlet header
			mesh_header.meshlet_header_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.meshlet_header_buffer_offset % 4 == 0);
			for (c_auto& meshlet_header_buffer : meshlet_header_buffer_arr)
			{
				buf.write_bytes(meshlet_header_buffer.data(), meshlet_header_buffer.byte_size());
			}

			// meshlet
			mesh_header.meshlet_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.meshlet_buffer_offset % 4 == 0);
			for (auto global_index_offset_base = 0u, local_index_offset_base = 0u;
				 auto&& [meshlet_buffer, meshlet_global_index_buffer, meshlet_local_index_buffer] :
				 std::views::zip(meshlet_buffer_arr, meshlet_global_index_buffer_arr, meshlet_local_index_buffer_arr))
			{
				for (auto& mshlt : meshlet_buffer)
				{
					mshlt.global_index_offset += global_index_offset_base;
					mshlt.local_index_offset  += local_index_offset_base;
				}

				global_index_offset_base += meshlet_global_index_buffer.size<uint32>();
				local_index_offset_base	 += meshlet_local_index_buffer.size<uint32>();

				buf.write_bytes(meshlet_buffer.data(), meshlet_buffer.byte_size());
			}

			// global index
			mesh_header.global_vertex_index_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.global_vertex_index_buffer_offset % 4 == 0);
			for (auto vertex_offset = 0u;
				 auto&& [meshlet_global_index_buffer, vertex_buffer] :
				 std::views::zip(meshlet_global_index_buffer_arr, vertex_buffer_arr))
			{
				for (auto& mshlt_global_idx : meshlet_global_index_buffer)
				{
					mshlt_global_idx += vertex_offset;
				}

				vertex_offset += vertex_buffer.size<uint32>();

				buf.write_bytes(meshlet_global_index_buffer.data(), meshlet_global_index_buffer.byte_size());
			}

			// local index
			mesh_header.local_vertex_index_buffer_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.local_vertex_index_buffer_offset % 4 == 0);
			for (c_auto& meshlet_local_index_buffer : meshlet_local_index_buffer_arr)
			{
				buf.write_bytes(meshlet_local_index_buffer.data(), meshlet_local_index_buffer.byte_size());
			}

			// align_up, typeof local_index : uint8
			while ((buf.size() - base) % 4 != 0u)
			{
				buf.write(uint8{});
			}

			// submesh data
			mesh_header.submesh_data_offset = buf.size<uint32>() - base;
			AGE_ASSERT(mesh_header.submesh_data_offset % 4 == 0);

			// submesh_meshlet_id_offset
			for (auto meshlet_id_offset = 0u;
				 c_auto& meshlet_buffer : meshlet_buffer_arr)
			{
				buf.write(uint32{ meshlet_id_offset });
				meshlet_id_offset += meshlet_buffer.size<uint32>();
			}

			// submesh_primitive_id_offset
			for (auto primitive_id_offset = 0u;
				 c_auto& meshlet_local_index_buffer : meshlet_local_index_buffer_arr)
			{
				buf.write(uint32{ primitive_id_offset });
				primitive_id_offset += meshlet_local_index_buffer.size<uint32>() / 3;

				AGE_ASSERT(meshlet_local_index_buffer.size<uint32>() % 3 == 0u);
			}

			// submesh_flags
			for (c_auto& desc : descs)
			{
				buf.write(cast_to<uint16>(to_idx<uint16>(desc.raster_mode) | (to_idx<uint16>(desc.rt_alpha_test_mode) << 8u)));
			}
			if (is_odd(submesh_count))
			{
				buf.write(uint16{});
			}

			mesh_header.meshlet_count		  = meshlet_count;
			mesh_header.submesh_count		  = submesh_count;
			mesh_header.aabb_min			  = aabb_min;
			mesh_header.aabb_size			  = aabb_size;
			mesh_header.vertex_kind_and_extra = to_idx(e_kind);	   // omm is disabled

			AGE_ASSERT((buf.size() - base) % 4u == 0u);

			asset_header.meshlet_buffer_byte_size = buf.size() - base;
			asset_header.index_count			  = local_index_buffer_size;
			asset_header.pos_count				  = vertex_count;

			// rt index buffer
			c_auto idx_buffer_offset = buf.size();

			for (auto global_index_offset = 0u, local_index_offset = 0u;
				 const auto&& [meshlet_buffer, meshlet_global_index_buffer, meshlet_local_index_buffer] :
				 std::views::zip(meshlet_buffer_arr, meshlet_global_index_buffer_arr, meshlet_local_index_buffer_arr))
			{
				for (c_auto& mshlt : meshlet_buffer)
				{
					for (c_auto i : std::views::iota(mshlt.local_index_offset) | std::views::take(mshlt.primitive_count * 3))
					{
						buf.write_at(idx_buffer_offset + i * sizeof(uint32),
									 meshlet_global_index_buffer[mshlt.global_index_offset - global_index_offset + meshlet_local_index_buffer[i - local_index_offset]]);
					}
				}

				global_index_offset += meshlet_global_index_buffer.size<uint32>();
				local_index_offset	+= meshlet_local_index_buffer.size<uint32>();
			}


			buf.move_write_pos(idx_buffer_offset + sizeof(uint32) * local_index_buffer_size);

			// pos buffer for blas build
			for (auto i : views::loop(vertex_count))
			{
				e::visit(e_kind, AGE_LAMBDA(<e::vertex_kind e_kind>(auto& buf, auto i, /*const vertex_fat& v_ref,*/ const float3& aabb_min, const float3& aabb_size), {
							 using t_vertex = t_vertex_kind<e_kind>;
							 auto v			= t_vertex{};
							 std::memcpy(&v, buf.data() + sizeof(asset_header) + sizeof(mesh_header) + sizeof(t_vertex) * i, sizeof(t_vertex));

							 c_auto v_fat = cvt_vertex_to<vertex_fat>(v, aabb_min, aabb_size);

							 // std::println("diff v_ref - v_meshlet : {}", v_ref.pos - v_fat.pos);
							 return buf.write(v_fat.pos);
						 }),
						 buf, i, aabb_min, aabb_size);
			}

			// for (c_auto& v : vertex_buffer)
			//{
			//	buf.write(v.pos);
			// }

			// rt_bake_mode
			for (c_auto& desc : descs)
			{
				buf.write(desc.rt_bake_mode);
			}
		}

		AGE_ASSERT(buf.capacity() == buf.size());

		buf.write_at(0, asset_header, mesh_header);
		c_auto f_header = get_default_file_header<e::kind::mesh_baked>(buf.size(), static_cast<uint8>(std::countr_zero(alignof(decltype(asset_header)))));
		write_asset_file(mesh_path.data(), f_header, buf.data());
		return;
	}

	void
	build_mesh_baked(const age::array<char, config::max_asset_path_len>& mesh_path, const primitive_desc& desc, e::vertex_kind e_kind) noexcept
	{
		return build_mesh_baked(mesh_path, { &desc, 1 }, e_kind);
	}

}	 // namespace age::asset::mesh_baked::detail
