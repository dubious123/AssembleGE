#pragma once
#include "age.hpp"

namespace age::graphics::defaults::srv_view_desc
{
	FORCE_INLINE decltype(auto)
	rt_acceleration_structure(resource_handle h_resource) noexcept
	{
		return D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format							 = DXGI_FORMAT_UNKNOWN,
			.ViewDimension					 = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
			.Shader4ComponentMapping		 = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.RaytracingAccelerationStructure = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV{ .Location = g::resource_vec[h_resource].p_resource->GetGPUVirtualAddress() }
		};
	}
}	 // namespace age::graphics::defaults::srv_view_desc