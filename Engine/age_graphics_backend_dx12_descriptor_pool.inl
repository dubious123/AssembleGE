#pragma once
#include "age.hpp"

// descriptor_heap member func
namespace age::graphics
{
	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	void
	descriptor_heap<heap_type, capacity, shader_visible>::init() noexcept
	{
		static_assert((capacity > 0) and (capacity <= D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2));
		static_assert(((heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) and (capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE)) is_false);

		D3D12_DESCRIPTOR_HEAP_DESC desc{
			.Type			= heap_type,
			.NumDescriptors = capacity,
			.Flags			= shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask		= 0
		};
		AGE_HR_CHECK(g::p_main_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&p_heap)));

		descriptor_size = g::p_main_device->GetDescriptorHandleIncrementSize(heap_type);

		c_auto fn_get_name = [] {
			switch (heap_type)
			{
			case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
				return shader_visible ? L"[CBV_SRV_UAV] Descriptor Heap" : L"[CBV_SRV_UAV] Descriptor Heap Non Shader Visible";
			case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
				return L"[RTV] Descriptor Heap";
			case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
				return L"[DSV] Descriptor Heap";
			case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				return L"[SAMPLER] Descriptor Heap";
			default:
				return L"Unknown";
			}
		};

		p_heap->SetName(fn_get_name());

		if constexpr (shader_visible)
		{
			h_start.h_cpu = p_heap->GetCPUDescriptorHandleForHeapStart();
			h_start.h_gpu = p_heap->GetGPUDescriptorHandleForHeapStart();
		}
		else
		{
			h_start.h_cpu = p_heap->GetCPUDescriptorHandleForHeapStart();
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	void
	descriptor_heap<heap_type, capacity, shader_visible>::deinit() noexcept
	{
		p_heap->Release();

		if constexpr (age::config::debug_mode)
		{
			desc_idx_pool.debug_validate();
			desc_idx_pool.cleanup();
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	descriptor_heap<heap_type, capacity, shader_visible>::handle<shader_visible>
	descriptor_heap<heap_type, capacity, shader_visible>::pop() noexcept
	{
		auto idx = desc_idx_pool.pop();

		if constexpr (shader_visible)
		{
			return handle<shader_visible>{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = h_start.h_cpu.ptr + descriptor_size * idx },
				.h_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = h_start.h_gpu.ptr + descriptor_size * idx },
				AGE_DEBUG_ONLY(.idx = idx)
			};
		}
		else
		{
			return handle<shader_visible>{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = h_start.h_cpu.ptr + descriptor_size * idx },
				AGE_DEBUG_ONLY(.idx = idx)
			};
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	descriptor_heap<heap_type, capacity, shader_visible>::handle<shader_visible>
	descriptor_heap<heap_type, capacity, shader_visible>::get(uint32 idx) noexcept
	{
		desc_idx_pool.get(idx);

		if constexpr (shader_visible)
		{
			return handle<shader_visible>{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = h_start.h_cpu.ptr + descriptor_size * idx },
				.h_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = h_start.h_gpu.ptr + descriptor_size * idx },
				AGE_DEBUG_ONLY(.idx = idx)
			};
		}
		else
		{
			return handle<shader_visible>{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = h_start.h_cpu.ptr + descriptor_size * idx },
				AGE_DEBUG_ONLY(.idx = idx)
			};
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	uint32
	descriptor_heap<heap_type, capacity, shader_visible>::calc_idx(handle<shader_visible> h) const noexcept
	{
		auto diff = h.h_cpu.ptr - h_start.h_cpu.ptr;
		auto idx  = static_cast<uint32>(diff / descriptor_size);

		AGE_ASSERT((diff % descriptor_size) == 0);
		AGE_ASSERT(idx == h.idx);

		return idx;
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	void
	descriptor_heap<heap_type, capacity, shader_visible>::push(handle<shader_visible> h) noexcept
	{
		desc_idx_pool.push(calc_idx(h));
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32 capacity, bool shader_visible>
	uint32
	descriptor_heap<heap_type, capacity, shader_visible>::count() const noexcept
	{
		return capacity - desc_idx_pool.free_count;
	}
}	 // namespace age::graphics

// helpers
namespace age::graphics
{
	void
	pop_descriptor(auto& h_descriptor_out) noexcept
	{
		using t_descriptor_handle = BARE_OF(h_descriptor_out);

		if constexpr (std::is_same_v<t_descriptor_handle, cbv_desc_handle>
					  or std::is_same_v<t_descriptor_handle, srv_desc_handle>
					  or std::is_same_v<t_descriptor_handle, uav_desc_handle>)
		{
			c_auto handle		   = g::cbv_srv_uav_desc_heap.pop();
			h_descriptor_out.h_cpu = handle.h_cpu;
			h_descriptor_out.h_gpu = handle.h_gpu;
			AGE_DEBUG_ONLY(h_descriptor_out.idx = handle.idx);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, sampler_desc_handle>)
		{
			c_auto handle		   = g::sampler_desc_heap.pop();
			h_descriptor_out.h_cpu = handle.h_cpu;
			h_descriptor_out.h_gpu = handle.h_gpu;
			AGE_DEBUG_ONLY(h_descriptor_out.idx = handle.idx);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, rtv_desc_handle>)
		{
			c_auto handle		   = g::rtv_desc_heap.pop();
			h_descriptor_out.h_cpu = handle.h_cpu;
			AGE_DEBUG_ONLY(h_descriptor_out.idx = handle.idx);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, dsv_desc_handle>)
		{
			c_auto handle		   = g::dsv_desc_heap.pop();
			h_descriptor_out.h_cpu = handle.h_cpu;
			AGE_DEBUG_ONLY(h_descriptor_out.idx = handle.idx);
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, clear_uav_desc_handle>)
		{
			c_auto handle	  = g::cbv_srv_uav_desc_heap.pop();
			c_auto handle_nsv = g::cbv_srv_uav_desc_heap_non_shader_visible.pop();

			h_descriptor_out.h_cpu					  = handle.h_cpu;
			h_descriptor_out.h_gpu					  = handle.h_gpu;
			h_descriptor_out.h_cpu_non_shader_visible = handle_nsv.h_cpu;
			AGE_DEBUG_ONLY(h_descriptor_out.idx = handle.idx);
			AGE_DEBUG_ONLY(h_descriptor_out.idx_non_shader_visible = handle_nsv.idx);
		}
		else
		{
			static_assert(false, "invalid descriptor handle type");
		}
	}

	template <typename t_desc>
	t_desc
	pop_descriptor() noexcept
	{
		auto res = t_desc{};
		pop_descriptor(res);
		return res;
	}

	void
	push_descriptor(c_auto& h_descriptor) noexcept
	{
		using t_descriptor_handle = BARE_OF(h_descriptor);

		if constexpr (std::is_same_v<t_descriptor_handle, cbv_desc_handle>
					  or std::is_same_v<t_descriptor_handle, srv_desc_handle>
					  or std::is_same_v<t_descriptor_handle, uav_desc_handle>)
		{
			g::cbv_srv_uav_desc_heap.push({ h_descriptor.h_cpu, h_descriptor.h_gpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, rtv_desc_handle>)
		{
			g::rtv_desc_heap.push({ h_descriptor.h_cpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, dsv_desc_handle>)
		{
			g::dsv_desc_heap.push({ h_descriptor.h_cpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, sampler_desc_handle>)
		{
			g::sampler_desc_heap.push({ h_descriptor.h_cpu, h_descriptor.h_gpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, clear_uav_desc_handle>)
		{
			g::cbv_srv_uav_desc_heap.push({ h_descriptor.h_cpu, h_descriptor.h_gpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
			g::cbv_srv_uav_desc_heap_non_shader_visible.push({ h_descriptor.h_cpu_non_shader_visible, AGE_DEBUG_ONLY(h_descriptor.idx_non_shader_visible) });
		}
		else
		{
			static_assert(false, "invalid descriptor handle type");
		}
	}

	uint32
	calc_desc_idx(c_auto& h_descriptor) noexcept
	{
		using t_descriptor_handle = BARE_OF(h_descriptor);

		if constexpr (std::is_same_v<t_descriptor_handle, cbv_desc_handle>
					  or std::is_same_v<t_descriptor_handle, srv_desc_handle>
					  or std::is_same_v<t_descriptor_handle, uav_desc_handle>)
		{
			return g::cbv_srv_uav_desc_heap.calc_idx({ h_descriptor.h_cpu, h_descriptor.h_gpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, rtv_desc_handle>)
		{
			return g::rtv_desc_heap.calc_idx({ h_descriptor.h_cpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, dsv_desc_handle>)
		{
			return g::dsv_desc_heap.calc_idx({ h_descriptor.h_cpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, sampler_desc_handle>)
		{
			return g::sampler_desc_heap.calc_idx({ h_descriptor.h_cpu, h_descriptor.h_gpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else if constexpr (std::is_same_v<t_descriptor_handle, clear_uav_desc_handle>)
		{
			return g::cbv_srv_uav_desc_heap.calc_idx({ h_descriptor.h_cpu, h_descriptor.h_gpu, AGE_DEBUG_ONLY(h_descriptor.idx) });
		}
		else
		{
			static_assert(false, "invalid descriptor handle type");
		}

		AGE_UNREACHABLE();
	}
}	 // namespace age::graphics