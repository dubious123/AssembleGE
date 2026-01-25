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
	create_resource(const d3d12_resource_desc& desc) noexcept
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
		//	g::heap, offset, desc_in.desc,
		//	desc_in.initial_state, &desc_in.clear_value, IID_PPV_ARGS(&handle.p_resource)));

		return resource_handle{ idx };
	}

	void
	release_resource(resource_handle h_resource) noexcept
	{
		g::resource_vec[h_resource].release();
		g::resource_vec.remove(h_resource);
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	FORCE_INLINE void
	create_view(const ID3D12Resource& res, const auto& h_desc, const auto& view_desc) noexcept
	{
		using t_descriptor_handle = std::remove_cvref_t<decltype(h_desc)>;
		using t_view_desc		  = std::remove_cvref_t<decltype(view_desc)>;

		if constexpr (std::is_same_v<t_descriptor_handle, age::graphics::cbv_desc_handle>
					  and std::is_same_v<t_view_desc, D3D12_CONSTANT_BUFFER_VIEW_DESC>)
		{
			g::p_main_device->CreateConstantBufferView(const_cast<ID3D12Resource*>(&res), &view_desc, h_desc.h_cpu);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, age::graphics::srv_desc_handle>
						   and std::is_same_v<t_view_desc, D3D12_SHADER_RESOURCE_VIEW_DESC>)
		{
			g::p_main_device->CreateShaderResourceView(const_cast<ID3D12Resource*>(&res), &view_desc, h_desc.h_cpu);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, age::graphics::uav_desc_handle>
						   and std::is_same_v<t_view_desc, D3D12_UNORDERED_ACCESS_VIEW_DESC>)
		{
			g::p_main_device->CreateUnorderedAccessView(const_cast<ID3D12Resource*>(&res), &view_desc, h_desc.h_cpu);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, age::graphics::rtv_desc_handle>
						   and std::is_same_v<t_view_desc, D3D12_RENDER_TARGET_VIEW_DESC>)
		{
			g::p_main_device->CreateRenderTargetView(const_cast<ID3D12Resource*>(&res), &view_desc, h_desc.h_cpu);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, age::graphics::dsv_desc_handle>
						   and std::is_same_v<t_view_desc, D3D12_DEPTH_STENCIL_VIEW_DESC>)
		{
			g::p_main_device->CreateDepthStencilView(const_cast<ID3D12Resource*>(&res), &view_desc, h_desc.h_cpu);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, age::graphics::sampler_desc_handle>
						   and std::is_same_v<t_view_desc, D3D12_SAMPLER_DESC>)
		{
			static_assert(false, "todo");
		}
		else
		{
			static_assert(false, "invalid descriptor handle type or desc type");
		}
	}
}	 // namespace age::graphics::resource


#endif