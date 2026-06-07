#pragma once
#include "age.hpp"

#define helper(name, ...)                                    \
	FORCE_INLINE decltype(auto)                              \
	name(resource_handle h_resource, auto&&... arg) noexcept \
	{                                                        \
		return name(h_resource->p_resource, FWD(arg)...);    \
	}

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

// copy location
namespace age::graphics::defaults
{
	namespace copy_location
	{
		helper(src);
		helper(src_placed);
		helper(src_subresource);
		helper(dst);
		helper(dst_subresource);
		helper(dst_placed);
	}	 // namespace copy_location
}	 // namespace age::graphics::defaults

#undef helper