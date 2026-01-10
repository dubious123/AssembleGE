#include "age_pch.hpp"
#include "age.hpp"
#if defined AGE_GRAPHICS_BACKEND_DX12
namespace age::graphics
{
	void
	resource_barrier::add_transition(ID3D12Resource&	   resource,
									 D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after,
									 D3D12_RESOURCE_BARRIER_FLAGS flags,
									 uint32						  subresource) noexcept
	{
		this->barrier_arr[barrier_count] =
			D3D12_RESOURCE_BARRIER{
				.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags		= flags,
				.Transition = {
					.pResource	 = &resource,
					.Subresource = subresource,
					.StateBefore = state_before,
					.StateAfter	 = state_after,
				}
			};

		++barrier_count;
	}

	void
	resource_barrier::add_uav(ID3D12Resource& resource, D3D12_RESOURCE_BARRIER_FLAGS flags) noexcept
	{
		this->barrier_arr[barrier_count] =
			D3D12_RESOURCE_BARRIER{
				.Type  = D3D12_RESOURCE_BARRIER_TYPE_UAV,
				.Flags = flags,
				.UAV   = {
					  .pResource = &resource }
			};

		++barrier_count;
	}

	void
	resource_barrier::add_aliasing(ID3D12Resource& resource_before, ID3D12Resource& resource_after,
								   D3D12_RESOURCE_BARRIER_FLAGS flags) noexcept
	{
		this->barrier_arr[barrier_count] =
			D3D12_RESOURCE_BARRIER{
				.Type	  = D3D12_RESOURCE_BARRIER_TYPE_ALIASING,
				.Flags	  = flags,
				.Aliasing = {
					.pResourceBefore = &resource_before,
					.pResourceAfter	 = &resource_after,
				}
			};

		++barrier_count;
	}

	void
	resource_barrier::apply_and_reset(ID3D12GraphicsCommandList9& cmd_list) noexcept
	{
		cmd_list.ResourceBarrier(barrier_count, barrier_arr.data());
		barrier_count = 0;
	}

	void
	resource_barrier::apply(ID3D12GraphicsCommandList9& cmd_list) noexcept
	{
		cmd_list.ResourceBarrier(barrier_count, barrier_arr.data());
	}

	void
	resource_barrier::reset() noexcept
	{
		barrier_count = 0;
	}
}	 // namespace age::graphics
#endif