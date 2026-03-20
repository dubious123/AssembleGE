#include "age_pch.hpp"
#include "age.hpp"

namespace age::graphics::rt
{
	FORCE_INLINE auto*
	blas_buffer_handle::operator->() noexcept
	{
		return &g::blas_buffer_data_vec[id];
	}

	FORCE_INLINE c_auto*
	blas_buffer_handle::operator->() const noexcept
	{
		return &g::blas_buffer_data_vec[id];
	}

	FORCE_INLINE void
	blas_buffer_data::set_name(const wchar_t* p_name) noexcept
	{
		h_resource->p_resource->SetName(p_name);
	}
}	 // namespace age::graphics::rt

namespace age::graphics::rt
{
	FORCE_INLINE auto
	blas_data::get_va() const noexcept
	{
		return h_blas_buffer->h_resource->get_va() + h_blas_buffer->blas_entry_vec[blas_entry_id].offset;
	}

	FORCE_INLINE auto*
	blas_handle::operator->() noexcept
	{
		return &g::blas_data_vec[id];
	}

	FORCE_INLINE c_auto*
	blas_handle::operator->() const noexcept
	{
		return &g::blas_data_vec[id];
	}
}	 // namespace age::graphics::rt

namespace age::graphics::rt
{
	namespace detail
	{
		resource_handle
		create_scratch_buffer(std::size_t byte_size) noexcept
		{
			return resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(byte_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
		}

		resource_handle
		create_blas_buffer(std::size_t byte_size) noexcept
		{
			auto h_res = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_rt(byte_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_res->set_name(L"blas_buffer");

			return h_res;
		}
	}	 // namespace detail

	void
	init() noexcept
	{
		g::h_rt_blas_scratch_buffer = detail::create_scratch_buffer(1024u);

		g::h_rt_blas_scratch_buffer->set_name(L"rt_blas_scratch_buffer");
	}

	void
	deinit() noexcept
	{
		AGE_ASSERT(g::blas_buffer_data_vec.is_empty());
		AGE_ASSERT(g::blas_data_vec.is_empty());

		for (auto& data : g::blas_buffer_data_vec)
		{
			resource::release(data.h_resource);
		}

		g::blas_buffer_data_vec.clear();
		g::blas_data_vec.clear();

		resource::release(g::h_rt_blas_scratch_buffer);
	}
}	 // namespace age::graphics::rt

namespace age::graphics::rt
{
	blas_buffer_handle
	create_blas_buffer(std::size_t initial_size) noexcept
	{
		AGE_ASSERT(initial_size % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0);

		auto h_resource = detail::create_blas_buffer(initial_size);

		auto id = static_cast<t_blas_buffer_handle_id>(
			g::blas_buffer_data_vec.emplace_back(blas_buffer_data{
				.h_resource		= h_resource,
				.next_offset	= 0,
				.size			= static_cast<uint32>(initial_size),
				.blas_entry_vec = {} }));

		return blas_buffer_handle{ .id = id };
	}

	void
	release_blas_buffer(blas_buffer_handle& h_blas_buffer) noexcept
	{
		resource::release(h_blas_buffer->h_resource);

		AGE_ASSERT(h_blas_buffer->blas_entry_vec.is_empty());

		g::blas_buffer_data_vec.remove(h_blas_buffer);

		h_blas_buffer.id = get_invalid_id<t_blas_buffer_handle_id>();
	}

}	 // namespace age::graphics::rt

namespace age::graphics::rt
{
	blas_handle
	build_blas(blas_buffer_handle h_blas_buffer, auto&&... rt_geo_desc) noexcept
	{
		c_auto arr = std::array<const D3D12_RAYTRACING_GEOMETRY_DESC, sizeof...(rt_geo_desc)>{
			FWD(rt_geo_desc)...
		};

		auto inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
			.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
			.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
			.NumDescs		= static_cast<uint32>(arr.size()),
			.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY,
			.pGeometryDescs = arr.data()
		};

		auto prebuild = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO{};

		graphics::g::p_main_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

		c_auto blas_byte_size = static_cast<uint32>(util::align_up(prebuild.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

		if (g::h_rt_blas_scratch_buffer->buffer_size() < prebuild.ScratchDataSizeInBytes)
		{
			resource::release(g::h_rt_blas_scratch_buffer);
			g::h_rt_blas_scratch_buffer = detail::create_scratch_buffer(prebuild.ScratchDataSizeInBytes);
			g::h_rt_blas_scratch_buffer->set_name(L"rt_blas_scratch_buffer");
		}


		if (h_blas_buffer->size - h_blas_buffer->next_offset < blas_byte_size)
		{
			command::begin();

			c_auto new_size		  = std::max(h_blas_buffer->next_offset + blas_byte_size, h_blas_buffer->size * 2);
			auto   h_resource_new = detail::create_blas_buffer(new_size);
			auto   offset		  = 0u;

			for (auto& entry : h_blas_buffer->blas_entry_vec)
			{
				command::copy_rt_acceleration_structure(
					h_resource_new->get_va() + offset,
					h_blas_buffer->h_resource->get_va() + entry.offset,
					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE);

				entry.offset  = offset;
				offset		 += entry.size;
			}

			command::execute_and_wait();

			resource::release(h_blas_buffer->h_resource);
			h_blas_buffer->h_resource  = h_resource_new;
			h_blas_buffer->next_offset = offset;
			h_blas_buffer->size		   = new_size;
		}

		auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
			.DestAccelerationStructureData	  = h_blas_buffer->h_resource->get_va() + h_blas_buffer->next_offset,
			.Inputs							  = inputs,
			.ScratchAccelerationStructureData = g::h_rt_blas_scratch_buffer->get_va(),
		};
		command::begin();

		command::build_rt_acceleration_structure(&build_desc, 0, nullptr);

		command::execute_and_wait();

		auto entry_id = h_blas_buffer->blas_entry_vec.emplace_back(
			blas_buffer_data::blas_entry{
				.offset = h_blas_buffer->next_offset,
				.size	= static_cast<uint32>(blas_byte_size) });

		auto blas_id = g::blas_data_vec.emplace_back(
			blas_data{
				.h_blas_buffer = h_blas_buffer,
				.blas_entry_id = entry_id });

		h_blas_buffer->next_offset += static_cast<uint32>(blas_byte_size);


		return blas_handle{ .id = static_cast<t_blas_handle_id>(blas_id) };
	}

	FORCE_INLINE
	std::tuple<uint64, uint64>
	query_tlas_size(uint32 instance_count) noexcept
	{
		c_auto inputs = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{
			.Type		 = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
			.Flags		 = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
			.NumDescs	 = instance_count,
			.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY,
		};

		auto prebuild = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO{};
		g::p_main_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);

		return { prebuild.ResultDataMaxSizeInBytes, prebuild.ScratchDataSizeInBytes };
	}

	FORCE_INLINE void
	build_tlas(resource_handle h_tlas_buffer, resource_handle h_tlas_scratch_buffer, resource_handle h_instance_buffer, uint32 instance_count) noexcept
	{
		c_auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
			.DestAccelerationStructureData = h_tlas_buffer->get_va(),
			.Inputs						   = {
				.Type		   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
				.Flags		   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
				.NumDescs	   = instance_count,
				.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY,
				.InstanceDescs = h_instance_buffer->get_va(),
			},
			.ScratchAccelerationStructureData = h_tlas_scratch_buffer->get_va(),
		};

		command::build_rt_acceleration_structure(&build_desc, 0, nullptr);
	}

	void
	release_blas(blas_handle& h_blas) noexcept
	{
		auto& h_blas_buffer = h_blas->h_blas_buffer;

		h_blas_buffer->blas_entry_vec.remove(h_blas);

		g::blas_data_vec.remove(h_blas);

		h_blas.id = age::get_invalid_id<t_blas_handle_id>();
	}
}	 // namespace age::graphics::rt