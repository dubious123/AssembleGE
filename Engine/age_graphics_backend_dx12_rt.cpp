#include "age_pch.hpp"
#include "age.hpp"

namespace age::graphics::rt
{
	namespace detail
	{
		resource_handle
		create_scratch_buffer(std::size_t byte_size, const wchar_t* p_name) noexcept
		{
			auto h_res = resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_uav(byte_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });

			h_res->set_name(p_name);

			return h_res;
		}

		resource_handle
		create_blas_buffer(std::size_t byte_size) noexcept
		{
			return resource::create_committed(
				{ .d3d12_resource_desc = defaults::resource_desc::buffer_rt(byte_size),
				  .initial_layout	   = D3D12_BARRIER_LAYOUT_UNDEFINED,
				  .heap_memory_kind	   = e::memory_kind::gpu_only,
				  .has_clear_value	   = false });
		}
	}	 // namespace detail

	void
	init() noexcept
	{
		g::h_rt_blas_scratch_buffer = detail::create_scratch_buffer(1024u, L"rt_blas_scratch_buffer");
	}

	void
	deinit() noexcept
	{
		resource::release(g::h_rt_blas_scratch_buffer);
	}
}	 // namespace age::graphics::rt

namespace age::graphics::rt
{
	resource_handle
	create_blas_buffer(std::size_t initial_size) noexcept
	{
		AGE_ASSERT(initial_size % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT == 0);

		return detail::create_blas_buffer(initial_size);
	}
}	 // namespace age::graphics::rt

namespace age::graphics::rt
{
	resource_handle
	build_blas(auto&&... rt_geo_desc) noexcept
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
			g::h_rt_blas_scratch_buffer = detail::create_scratch_buffer(prebuild.ScratchDataSizeInBytes, L"rt_blas_scratch_buffer");
		}

		auto h_blas = detail::create_blas_buffer(blas_byte_size);

		auto build_desc = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC{
			.DestAccelerationStructureData	  = h_blas->get_va(),
			.Inputs							  = inputs,
			.ScratchAccelerationStructureData = g::h_rt_blas_scratch_buffer->get_va(),
		};

		command::begin();

		command::build_rt_acceleration_structure(&build_desc, 0, nullptr);

		command::execute_and_wait();

		return h_blas;
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
}	 // namespace age::graphics::rt