#include "age_pch.hpp"
#include "age.hpp"
#if defined AGE_GRAPHICS_BACKEND_DX12

namespace age::graphics
{
	FORCE_INLINE auto*
	resource_handle::operator->() noexcept
	{
		return &g::resource_vec[id];
	}
}	 // namespace age::graphics

namespace age::graphics::resource
{
	FORCE_INLINE resource_mapping*
	mapping_handle::operator->() noexcept
	{
		return &g::resource_mapping_vec[id];
	}

}	 // namespace age::graphics::resource

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
		for (auto& m : g::resource_mapping_vec)
		{
			m.h_resource->p_resource->Unmap(0, nullptr);
			m.h_resource->map_count--;
		}

		for (auto& resource : g::resource_vec)
		{
			AGE_ASSERT(resource.map_count == 0, "all mapping should be unmapped");
			resource.p_resource->Release();
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
	create_committed(const resource_create_desc& desc) noexcept
	{
		auto* p_resource = (ID3D12Resource*)nullptr;

		auto heap_prop = defaults::heap_properties::committed_heap(desc.heap_memory_kind);

		if (bool is_not_rt_ds = (desc.d3d12_resource_desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) == 0)
		{
			AGE_ASSERT(desc.has_clear_value is_false);
		}

		AGE_HR_CHECK(g::p_main_device->CreateCommittedResource(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&desc.d3d12_resource_desc,
			desc.initial_state,
			desc.has_clear_value ? &desc.clear_value : nullptr,
			IID_PPV_ARGS(&p_resource)));

		return resource_handle{ .id = g::resource_vec.emplace_back(p_resource) };
	}

	resource_handle
	create_placed(const resource_create_desc& desc, ID3D12Heap& heap, uint64 offset) noexcept
	{
		auto* p_resource = (ID3D12Resource*)nullptr;

		if (bool is_not_rt_ds = (desc.d3d12_resource_desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) == 0)
		{
			AGE_ASSERT(desc.has_clear_value is_false);
		}

		{
			auto info = g::p_main_device->GetResourceAllocationInfo(0, 1, &desc.d3d12_resource_desc);
			AGE_ASSERT((offset % info.Alignment) == 0);
		}

		AGE_HR_CHECK(g::p_main_device->CreatePlacedResource(
			&heap,
			offset,
			&desc.d3d12_resource_desc,
			desc.initial_state,
			desc.has_clear_value ? &desc.clear_value : nullptr,
			IID_PPV_ARGS(&p_resource)));

		return resource_handle{ .id = g::resource_vec.emplace_back(p_resource) };
	}

	void
	release_resource(resource_handle h_resource) noexcept
	{
		g::resource_vec[h_resource].p_resource->Release();
		g::resource_vec.remove(h_resource);
	}

	mapping_handle
	map_all(resource_handle h_resource) noexcept
	{
		auto cpu_read_range = D3D12_RANGE{};

		void* cpu_address = nullptr;
		AGE_HR_CHECK(h_resource->p_resource->Map(0, &cpu_read_range, &cpu_address));
		AGE_ASSERT(cpu_address is_not_nullptr);

		return mapping_handle{ .id = g::resource_mapping_vec.emplace_back(cpu_address, h_resource) };
	}

	void
	unmap(mapping_handle h_map) noexcept
	{
		h_map->h_resource->p_resource->Unmap(0, nullptr);
		g::resource_mapping_vec.remove(h_map);
	}

	void
	unmap_and_release(mapping_handle h_map) noexcept
	{
		h_map->h_resource->p_resource->Unmap(0, nullptr);

		AGE_ASSERT(h_map->h_resource->map_count == 0);

		release_resource(h_map->h_resource);

		g::resource_mapping_vec.remove(h_map);
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	mapping_handle
	create_buffer_committed(uint32				  buffer_byte_size,
							const void*			  p_data,
							e::memory_kind		  kind,
							D3D12_RESOURCE_STATES state,
							D3D12_RESOURCE_FLAGS  flags) noexcept
	{
		auto h_resource = create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer(buffer_byte_size, flags),
			  .initial_state	   = state,
			  .heap_memory_kind	   = kind,
			  .has_clear_value	   = false });

		auto h_map = map_all(h_resource);

		if (p_data is_not_nullptr)
		{
			switch (kind)
			{
			case age::graphics::resource::e::memory_kind::gpu_only:
				AGE_UNREACHABLE();
				break;
			case age::graphics::resource::e::memory_kind::cpu_to_gpu:
				AGE_UNREACHABLE();
				break;
			case age::graphics::resource::e::memory_kind::gpu_to_cpu:
				AGE_UNREACHABLE();
				break;
			case age::graphics::resource::e::memory_kind::cpu_to_gpu_direct:
				std::memcpy(h_map->ptr, p_data, buffer_byte_size);
				break;
			case age::graphics::resource::e::memory_kind::count:
				AGE_UNREACHABLE();
				break;
			default:
				AGE_UNREACHABLE();
				break;
			}
		}

		return h_map;
	}

	mapping_handle
	create_buffer_placed(uint32				   buffer_byte_size,
						 ID3D12Heap&		   heap,
						 uint64				   offset,
						 const void*		   p_data,
						 e::memory_kind		   kind,
						 D3D12_RESOURCE_STATES state,
						 D3D12_RESOURCE_FLAGS  flags) noexcept
	{
		auto h_resource = create_placed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer(buffer_byte_size, flags),
			  .initial_state	   = state,
			  .heap_memory_kind	   = kind,
			  .has_clear_value	   = false },
			heap,
			offset);

		auto h_map = map_all(h_resource);

		if (p_data is_not_nullptr)
		{
			switch (kind)
			{
			case age::graphics::resource::e::memory_kind::gpu_only:
				AGE_UNREACHABLE();
				break;
			case age::graphics::resource::e::memory_kind::cpu_to_gpu:
				AGE_UNREACHABLE();
				break;
			case age::graphics::resource::e::memory_kind::gpu_to_cpu:
				AGE_UNREACHABLE();
				break;
			case age::graphics::resource::e::memory_kind::cpu_to_gpu_direct:
				std::memcpy(h_map->ptr, p_data, buffer_byte_size);
				break;
			case age::graphics::resource::e::memory_kind::count:
				AGE_UNREACHABLE();
				break;
			default:
				AGE_UNREACHABLE();
				break;
			}
		}

		return h_map;
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	FORCE_INLINE void
	create_view(const ID3D12Resource& res, const auto& h_desc, const auto& view_desc) noexcept
	{
		using t_descriptor_handle = BARE_OF(h_desc);
		using t_view_desc		  = BARE_OF(view_desc);

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

	FORCE_INLINE void
	create_view(const graphics::resource_handle& h_resource, const auto& h_desc, const auto& view_desc) noexcept
	{
		create_view(*g::resource_vec[h_resource].p_resource, h_desc, view_desc);
	}
}	 // namespace age::graphics::resource

#endif