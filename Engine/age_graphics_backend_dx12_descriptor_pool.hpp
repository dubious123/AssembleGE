#pragma once

// descriptor_pool member func
namespace age::graphics
{
	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	FORCE_INLINE void
	descriptor_pool<heap_type, capacity>::init() noexcept
	{
		static_assert((capacity > 0) and (capacity <= D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2));
		static_assert(not((heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) and (capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE)));

		{
			D3D12_DESCRIPTOR_HEAP_DESC desc{
				.Type			= heap_type,
				.NumDescriptors = capacity,
				.Flags			= is_shader_visible() ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask		= 0
			};

			constexpr auto wstr_type = [] constexpr {
				switch (heap_type)
				{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
					return L"[CBV_SRV_UAV] Descriptor Heap";
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
					return L"[SAMPLER] Descriptor Heap";
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
					return L"[RTV] Descriptor Heap";
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					return L"[DSV] Descriptor Heap";
				default:
					return L"Unknown";
				}
			}();

			AGE_HR_CHECK(g::p_main_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&p_descriptor_heap)));
			p_descriptor_heap->SetName(wstr_type);
		}

		descriptor_size = g::p_main_device->GetDescriptorHandleIncrementSize(heap_type);

		if constexpr (is_shader_visible())
		{
			start_handle.h_cpu = p_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
			start_handle.h_gpu = p_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
		}
		else
		{
			start_handle.h_cpu = p_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	FORCE_INLINE void
	descriptor_pool<heap_type, capacity>::deinit() noexcept
	{
		p_descriptor_heap->Release();

		if constexpr (age::config::debug_mode)
		{
			desc_idx_pool.debug_validate();
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	descriptor_pool<heap_type, capacity>::t_descriptor_handle
	descriptor_pool<heap_type, capacity>::pop() noexcept
	{
		auto idx = desc_idx_pool.pop();

		if constexpr (is_shader_visible())
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				.h_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_gpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
		else
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	descriptor_pool<heap_type, capacity>::t_descriptor_handle
	descriptor_pool<heap_type, capacity>::get(uint32 idx) noexcept
	{
		desc_idx_pool.get(idx);

		if constexpr (is_shader_visible())
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				.h_gpu = D3D12_GPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_gpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
		else
		{
			return t_descriptor_handle{
				.h_cpu = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = start_handle.h_cpu.ptr + descriptor_size * idx },
				AGE_DEBUG_OP(.idx = idx)
			};
		}
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	void
	descriptor_pool<heap_type, capacity>::push(t_descriptor_handle h) noexcept
	{
		desc_idx_pool.push(this->calc_idx(h));
	}

	template <D3D12_DESCRIPTOR_HEAP_TYPE heap_type, std::size_t capacity>
	uint32
	descriptor_pool<heap_type, capacity>::calc_idx(t_descriptor_handle h) noexcept
	{
		auto diff = h.h_cpu.ptr - start_handle.h_cpu.ptr;
		auto idx  = static_cast<uint32>(diff / descriptor_size);

		AGE_ASSERT((diff % descriptor_size) == 0);
		AGE_ASSERT(idx == h.idx);


		return idx;
	}
}	 // namespace age::graphics