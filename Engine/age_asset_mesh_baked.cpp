#include "age_pch.hpp"
#include "age.hpp"

namespace age::asset
{
	mesh_baked_header
	mesh_baked::get_header() const noexcept
	{
		AGE_ASSERT(buffer.size() >= sizeof(mesh_baked_header));

		auto res = mesh_baked_header{};
		std::memcpy(&res, buffer.data(), sizeof(mesh_baked_header));
		return res;
	}

	std::span<const uint32>
	mesh_baked::get_global_vertex_index_buffer() const noexcept
	{
		c_auto	header = get_header();
		c_auto* ptr	   = reinterpret_cast<const uint32*>(buffer.data() + header.global_vertex_index_buffer_offset);

		c_auto count = (header.local_vertex_index_buffer_offset - header.global_vertex_index_buffer_offset) / sizeof(uint32);

		return std::span{ ptr, count };
	}

	std::span<const uint8>
	mesh_baked::get_local_vertex_index_buffer() const noexcept
	{
		c_auto	header = get_header();
		c_auto* ptr	   = reinterpret_cast<const uint8*>(buffer.data() + header.local_vertex_index_buffer_offset);

		auto count = 0u;

		for (c_auto& mslt : get_meshlet_buffer())
		{
			count += mslt.primitive_count;
		}

		return std::span{ ptr, count * 3 };
	}

	std::span<const meshlet>
	mesh_baked::get_meshlet_buffer() const noexcept
	{
		// todo, start_lifetime_as_array
		c_auto	header = get_header();
		c_auto* ptr	   = reinterpret_cast<const meshlet*>(buffer.data() + header.meshlet_buffer_offset);

		return std::span{ ptr, header.meshlet_count };
	}

	std::span<const meshlet_header>
	mesh_baked::get_mesh_header_buffer() const noexcept
	{
		c_auto	header = get_header();
		c_auto* ptr	   = reinterpret_cast<const meshlet_header*>(buffer.data() + header.meshlet_header_buffer_offset);

		return std::span{ ptr, header.meshlet_count };
	}

	template <cx_baked_vertex t>
	FORCE_INLINE
	std::span<const t>
	mesh_baked::get_vertex_buffer() const noexcept
	{
		c_auto	header = get_header();
		c_auto* ptr	   = reinterpret_cast<const t*>(buffer.data() + sizeof(mesh_baked_header));

		c_auto count = (header.meshlet_header_buffer_offset - sizeof(mesh_baked_header)) / sizeof(t);

		return std::span{ ptr, count };
	}

	age::dynamic_array<uint32>
	mesh_baked::gen_flat_index_arr() const noexcept
	{
		c_auto mshlt_buffer		   = get_meshlet_buffer();
		c_auto global_index_buffer = get_global_vertex_index_buffer();
		c_auto local_index_buffer  = get_local_vertex_index_buffer();

		auto res = age::dynamic_array<uint32>::gen_sized_default(local_index_buffer.size());

		for (c_auto& mshlt : mshlt_buffer)
		{
			for (c_auto i : std::views::iota(mshlt.primitive_offset) | std::views::take(mshlt.primitive_count * 3))
			{
				res[i] = global_index_buffer[mshlt.global_index_offset + local_index_buffer[i]];
			}
		}

		return res;
	}

	template <cx_baked_vertex t>
	age::dynamic_array<float3>
	mesh_baked::gen_vertex_pos_arr() const noexcept
	{
		c_auto header = get_header();

		c_auto vertex_buffer = get_vertex_buffer<t>();
		auto   res			 = age::dynamic_array<float3>::gen_sized_default(vertex_buffer.size());

		for (auto&& [i, v] : vertex_buffer | std::views::enumerate)
		{
			res[i] = cvt_vertex_to<vertex_fat>(v, header.aabb_min, header.aabb_size).pos;
		}

		return res;
	}
}	 // namespace age::asset