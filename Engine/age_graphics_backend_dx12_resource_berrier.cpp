#include "age_pch.hpp"
#include "age.hpp"
#if defined AGE_GRAPHICS_BACKEND_DX12
namespace age::graphics
{
	FORCE_INLINE void
	resource_barrier::add(const D3D12_RESOURCE_BARRIER& barrier) noexcept
	{
		this->barrier_arr[barrier_count] = barrier;
		++barrier_count;
	}

	void
	resource_barrier::add_transition(ID3D12Resource&	   resource,
									 D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after,
									 D3D12_RESOURCE_BARRIER_FLAGS flags,
									 uint32						  subresource) noexcept
	{
		this->add({ .Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= flags,
					.Transition = {
						.pResource	 = &resource,
						.Subresource = subresource,
						.StateBefore = state_before,
						.StateAfter	 = state_after,
					} });
	}

	void
	resource_barrier::add_uav(ID3D12Resource& resource, D3D12_RESOURCE_BARRIER_FLAGS flags) noexcept
	{
		this->add({ .Type  = D3D12_RESOURCE_BARRIER_TYPE_UAV,
					.Flags = flags,
					.UAV   = {
						.pResource = &resource } });
	}

	void
	resource_barrier::add_aliasing(ID3D12Resource& resource_before, ID3D12Resource& resource_after,
								   D3D12_RESOURCE_BARRIER_FLAGS flags) noexcept
	{
		this->add({ .Type	  = D3D12_RESOURCE_BARRIER_TYPE_ALIASING,
					.Flags	  = flags,
					.Aliasing = {
						.pResourceBefore = &resource_before,
						.pResourceAfter	 = &resource_after,
					} });
	}

	void
	resource_barrier::apply_and_reset(t_cmd_list& cmd_list) noexcept
	{
		cmd_list.ResourceBarrier(barrier_count, barrier_arr.data());
		barrier_count = 0;
	}

	void
	resource_barrier::apply(t_cmd_list& cmd_list) noexcept
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