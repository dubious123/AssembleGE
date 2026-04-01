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

	FORCE_INLINE c_auto*
	resource_handle::operator->() const noexcept
	{
		return &g::resource_vec[id];
	}

}	 // namespace age::graphics

namespace age::graphics
{
	FORCE_INLINE std::size_t
	d3d12_resource::buffer_size() const noexcept
	{
		return p_resource->GetDesc().Width;
	}

	FORCE_INLINE D3D12_GPU_VIRTUAL_ADDRESS
	d3d12_resource::get_va() const noexcept
	{
		return p_resource->GetGPUVirtualAddress();
	}

	void
	d3d12_resource::set_name(const wchar_t* name) noexcept
	{
		p_resource->SetName(name);
	}

	std::wstring
	d3d12_resource::get_name() const noexcept
	{
		wchar_t name[256]{};
		auto	size = static_cast<UINT>(sizeof(name));
		p_resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
		return std::wstring(name, size / sizeof(wchar_t));
	}
}	 // namespace age::graphics

namespace age::graphics
{
	FORCE_INLINE resource_mapping*
	mapping_handle::operator->() noexcept
	{
		return &g::resource_mapping_vec[id];
	}

	FORCE_INLINE const resource_mapping*
	mapping_handle::operator->() const noexcept
	{
		return &g::resource_mapping_vec[id];
	}

	FORCE_INLINE void
	resource_mapping::upload(const void* p_src, std::size_t size, std::size_t offset /* = 0u */) noexcept
	{
		std::memcpy(ptr + offset, p_src, size);
	}
}	 // namespace age::graphics

namespace age::graphics::resource
{
	void
	init() noexcept
	{
		g::h_upload_buffer = resource::create_buffer_committed(1024);
		// todo create heap
	}

	void
	deinit() noexcept
	{
		unmap_and_release(g::h_upload_buffer);

		for (auto i : std::views::iota(0u) | std::views::take(g::deferred_release_data_vec.size()) | std::views::reverse)
		{
			auto& data = g::deferred_release_data_vec[i];

			AGE_ASSERT(command::is_complete(data.kind, data.fence_value));

			resource::release(data.h_resource);
			g::deferred_release_data_vec.pop_back();
		}

		// if (g::resource_vec.is_empty() is_false)
		//{
		//	for (auto& r : g::resource_vec)
		//	{
		//		// todo better logging
		//		_putws(r.get_name().c_str());
		//	}
		// }

		AGE_ASSERT(g::resource_mapping_vec.is_empty(), "all mapping should be unmapped");
		AGE_ASSERT(g::resource_vec.is_empty(), "all resource should be released");
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

		g::resource_mapping_vec.clear();
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

		AGE_HR_CHECK(
			g::p_main_device->CreateCommittedResource3(
				&heap_prop,
				D3D12_HEAP_FLAG_NONE,
				&desc.d3d12_resource_desc,
				desc.initial_layout,
				desc.has_clear_value ? &desc.clear_value : nullptr,
				nullptr,
				0,
				nullptr,
				IID_PPV_ARGS(&p_resource)));

		c_auto res = d3d12_resource{
			.p_resource = p_resource,
			.desc		= desc
		};

		return resource_handle{ .id = g::resource_vec.emplace_back(res) };
	}

	template <auto n>
	std::array<resource_handle, n>
	create_committed(const resource_create_desc& desc) noexcept
	{
		auto result = std::array<resource_handle, n>{};
		for (auto& h : result)
		{
			h = create_committed(desc);
		}
		return result;
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
			auto info = g::p_main_device->GetResourceAllocationInfo2(0, 1, &desc.d3d12_resource_desc, nullptr);
			AGE_ASSERT((offset % info.Alignment) == 0);
		}

		AGE_HR_CHECK(g::p_main_device->CreatePlacedResource2(
			&heap,
			offset,
			&desc.d3d12_resource_desc,
			desc.initial_layout,
			desc.has_clear_value ? &desc.clear_value : nullptr,
			0,
			nullptr,
			IID_PPV_ARGS(&p_resource)));

		c_auto res = d3d12_resource{
			.p_resource = p_resource,
			.desc		= desc
		};

		return resource_handle{ .id = g::resource_vec.emplace_back(res) };
	}

	void
	release(resource_handle& h_resource) noexcept
	{
		g::resource_vec[h_resource].p_resource->Release();
		g::resource_vec.remove(h_resource);
		h_resource = {};
	}

	void
	release(std::span<resource_handle> h_resources) noexcept
	{
		for (auto& h : h_resources)
		{
			release(h);
		}
	}

	FORCE_INLINE void
	release_deferred(resource_handle& h_resource, e::queue_kind kind) noexcept
	{
		g::deferred_release_data_vec.emplace_back(
			deferred_release_data{
				.h_resource	 = h_resource,
				.kind		 = kind,
				.fence_value = command::current_fence_value(kind) });

		h_resource = {};
	}

	void
	process_deferred_releases() noexcept
	{
		for (auto i : std::views::iota(0u) | std::views::take(g::deferred_release_data_vec.size()) | std::views::reverse)
		{
			auto& data = g::deferred_release_data_vec[i];

			if (command::is_complete(data.kind, data.fence_value))
			{
				resource::release(data.h_resource);
				if (i != g::deferred_release_data_vec.size() - 1)
				{
					g::deferred_release_data_vec[i] = std::move(g::deferred_release_data_vec.back());
				}
				g::deferred_release_data_vec.pop_back();
			}
		}
	}

	mapping_handle
	map_all(resource_handle h_resource) noexcept
	{
		auto cpu_read_range = D3D12_RANGE{};

		auto p_cpu_address = (std::byte*)nullptr;
		AGE_HR_CHECK(h_resource->p_resource->Map(0, &cpu_read_range, (void**)&p_cpu_address));
		AGE_ASSERT(p_cpu_address is_not_nullptr);

		return mapping_handle{ .id = g::resource_mapping_vec.emplace_back(p_cpu_address, h_resource) };
	}

	void
	unmap(mapping_handle& h_map) noexcept
	{
		h_map->h_resource->p_resource->Unmap(0, nullptr);
		g::resource_mapping_vec.remove(h_map);

		h_map = {};
	}

	void
	unmap_and_release(mapping_handle& h_map) noexcept
	{
		h_map->h_resource->p_resource->Unmap(0, nullptr);

		AGE_ASSERT(h_map->h_resource->map_count == 0);

		release(h_map->h_resource);

		g::resource_mapping_vec.remove(h_map);

		h_map = {};
	}

	void
	unmap_and_release(std::span<mapping_handle> h_mappings) noexcept
	{
		for (auto& h : h_mappings)
		{
			unmap_and_release(h);
		}
	}

	FORCE_INLINE bool
	resize_buffer(resource_handle& h_resource, uint64 required_size) noexcept
	{
		if (h_resource->buffer_size() >= required_size)
		{
			return false;
		}

		c_auto new_size = std::max(h_resource->buffer_size() * 2, required_size);

		auto desc					   = h_resource->desc;
		desc.d3d12_resource_desc.Width = new_size;

		auto h_new = resource::create_committed(desc);

		wchar_t name[256]{};
		auto	size = static_cast<UINT>(sizeof(name));
		h_resource->p_resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
		h_new->set_name(name);

		release_deferred(h_resource);
		h_resource = h_new;

		return true;
	}

	FORCE_INLINE bool
	resize_buffer(mapping_handle& h_mapping, uint64 required_size) noexcept
	{
		if (h_mapping->h_resource->buffer_size() >= required_size)
		{
			return false;
		}

		auto h_resource = h_mapping->h_resource;

		unmap(h_mapping);

		c_auto new_size = std::max(h_resource->buffer_size() * 2, required_size);

		auto desc					   = h_resource->desc;
		desc.d3d12_resource_desc.Width = new_size;

		auto h_new = resource::create_committed(desc);

		h_new->set_name(h_resource->get_name().c_str());

		release_deferred(h_resource);
		h_resource = h_new;

		h_mapping = map_all(h_resource);

		return true;
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	mapping_handle
	create_buffer_committed(uint32				 buffer_byte_size,
							const void*			 p_data,
							e::memory_kind		 kind,
							D3D12_BARRIER_LAYOUT initial_layout,
							D3D12_RESOURCE_FLAGS flags) noexcept
	{
		auto h_resource = create_committed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer(buffer_byte_size, flags),
			  .initial_layout	   = initial_layout,
			  .heap_memory_kind	   = kind,
			  .has_clear_value	   = false });

		auto h_map = map_all(h_resource);

		if (p_data is_not_nullptr)
		{
			switch (kind)
			{
			case e::memory_kind::gpu_only:
				AGE_UNREACHABLE();
				break;
			case e::memory_kind::cpu_to_gpu:
				AGE_UNREACHABLE();
				break;
			case e::memory_kind::gpu_to_cpu:
				AGE_UNREACHABLE();
				break;
			case e::memory_kind::cpu_to_gpu_direct:
				std::memcpy(h_map->ptr, p_data, buffer_byte_size);
				break;
			default:
				AGE_UNREACHABLE();
				break;
			}
		}

		return h_map;
	}

	template <auto n>
	std::array<mapping_handle, n>
	create_buffer_committed(uint32				 buffer_byte_size,
							const void*			 p_data,
							e::memory_kind		 kind,
							D3D12_BARRIER_LAYOUT initial_layout,
							D3D12_RESOURCE_FLAGS flags) noexcept
	{
		auto result = std::array<mapping_handle, n>{};
		for (auto& h : result)
		{
			h = create_buffer_committed(buffer_byte_size, p_data, kind, initial_layout, flags);
		}
		return result;
	}

	mapping_handle
	create_buffer_placed(uint32				  buffer_byte_size,
						 ID3D12Heap&		  heap,
						 uint64				  offset,
						 const void*		  p_data,
						 e::memory_kind		  kind,
						 D3D12_BARRIER_LAYOUT initial_layout,
						 D3D12_RESOURCE_FLAGS flags) noexcept
	{
		auto h_resource = create_placed(
			{ .d3d12_resource_desc = defaults::resource_desc::buffer(buffer_byte_size, flags),
			  .initial_layout	   = initial_layout,
			  .heap_memory_kind	   = kind,
			  .has_clear_value	   = false },
			heap,
			offset);

		auto h_map = map_all(h_resource);

		if (p_data is_not_nullptr)
		{
			switch (kind)
			{
			case e::memory_kind::gpu_only:
				AGE_UNREACHABLE();
				break;
			case e::memory_kind::cpu_to_gpu:
				AGE_UNREACHABLE();
				break;
			case e::memory_kind::gpu_to_cpu:
				AGE_UNREACHABLE();
				break;
			case e::memory_kind::cpu_to_gpu_direct:
				std::memcpy(h_map->ptr, p_data, buffer_byte_size);
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
			g::p_main_device->CreateUnorderedAccessView(const_cast<ID3D12Resource*>(&res), nullptr, &view_desc, h_desc.h_cpu);
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

	FORCE_INLINE void
	create_view(const auto& h_desc, const auto& view_desc) noexcept
	{
		g::p_main_device->CreateShaderResourceView(nullptr, &view_desc, h_desc.h_cpu);
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	inline void
	set_name(std::span<resource_handle> rng, const wchar_t* fmt) noexcept
	{
		for (auto&& [i, h_resource] : rng | std::views::enumerate)
		{
			auto str = std::vformat(fmt, std::make_wformat_args(i));
			h_resource->set_name(str.c_str());
		}
	}

	inline void
	set_name(std::span<mapping_handle> rng, const wchar_t* fmt) noexcept
	{
		for (auto&& [i, h_mapping] : rng | std::views::enumerate)
		{
			auto str = std::vformat(fmt, std::make_wformat_args(i));
			h_mapping->h_resource->set_name(str.c_str());
		}
	}
}	 // namespace age::graphics::resource

namespace age::graphics::resource
{
	void
	upload_texture(resource_handle h_dst, const void* p_src_cpu, age::extent_2d<uint32> extent, DXGI_FORMAT dx12_format) noexcept
	{
		c_auto format_byte_size = format_size(dx12_format);
		c_auto row_pitch		= age::util::align_up<uint32>(extent.width * format_byte_size, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		c_auto total_size		= static_cast<uint32>(row_pitch) * extent.height;

		resize_buffer(g::h_upload_buffer, total_size);

		for (auto y : std::views::iota(0u, extent.height))
		{
			g::h_upload_buffer->upload(static_cast<const std::byte*>(p_src_cpu) + y * extent.width * format_byte_size,
									   extent.width * format_byte_size,
									   y * row_pitch);
		}

		c_auto src = defaults::copy_location::src(g::h_upload_buffer->h_resource->p_resource, extent, dx12_format, row_pitch);
		c_auto dst = defaults::copy_location::dst(h_dst->p_resource);

		command::copy_texture(&dst, 0, 0, 0, &src, nullptr);
	}
}	 // namespace age::graphics::resource

#endif