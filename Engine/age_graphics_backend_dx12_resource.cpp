#include "age_pch.hpp"
#include "age.hpp"
#if defined AGE_GRAPHICS_BACKEND_DX12

namespace age::graphics::resource
{
	void
	init() noexcept
	{
		// todo create heap
	}

	void
	deinit() noexcept
	{
		for (auto& resource : g::resource_vec)
		{
			resource.release();
		}

		if constexpr (age::config::debug_mode)
		{
			g::resource_vec.debug_validate();
		}

		g::resource_vec.clear();
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	resource_handle
	create_resource(d3d12_resource_desc desc) noexcept
	{
		// todo do createPlacedResource
		// do better
		auto idx = g::resource_vec.emplace_back();

		auto heap_properties = D3D12_HEAP_PROPERTIES{
			.Type				  = static_cast<D3D12_HEAP_TYPE>(desc.heap_memory_kind),
			.CPUPageProperty	  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask	  = 1,
			.VisibleNodeMask	  = 1
		};

		bool is_rt_ds = (desc.d3d12_desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0;

		if (not is_rt_ds)
		{
			AGE_ASSERT(desc.has_clear_value is_false);
		}

		AGE_HR_CHECK(g::p_main_device->CreateCommittedResource(
			&heap_properties, D3D12_HEAP_FLAG_NONE, &desc.d3d12_desc,
			desc.initial_state, desc.has_clear_value ? &desc.clear_value : nullptr, IID_PPV_ARGS(&g::resource_vec[idx].p_resource)));

		// AGE_HR_CHECK(g::p_main_device->CreatePlacedResource(
		//	g::heap, offset, desc.desc,
		//	desc.initial_state, &desc.clear_value, IID_PPV_ARGS(&handle.p_resource)));

		return resource_handle{ idx };
	}

	void
	release_resource(resource_handle handle) noexcept
	{
		g::resource_vec[handle.id].release();
		g::resource_vec.remove(handle.id);
	}
}	 // namespace age::graphics::resource

// resource_barrier
namespace age::graphics::resource
{

}

#endif