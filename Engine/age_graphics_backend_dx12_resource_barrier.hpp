#include "age.hpp"

namespace age::graphics::barrier
{
	FORCE_INLINE decltype(auto)
	present_to_rtv(ID3D12Resource* p_resource, D3D12_TEXTURE_BARRIER_FLAGS flag = {}) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = D3D12_BARRIER_SYNC_NONE,
			.SyncAfter	  = D3D12_BARRIER_SYNC_RENDER_TARGET,
			.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
			.AccessAfter  = D3D12_BARRIER_ACCESS_RENDER_TARGET,
			.LayoutBefore = D3D12_BARRIER_LAYOUT_PRESENT,
			.LayoutAfter  = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	rtv_to_present(ID3D12Resource* p_resource) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = D3D12_BARRIER_SYNC_RENDER_TARGET,
			.SyncAfter	  = D3D12_BARRIER_SYNC_NONE,
			.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET,
			.AccessAfter  = D3D12_BARRIER_ACCESS_NO_ACCESS,
			.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
			.LayoutAfter  = D3D12_BARRIER_LAYOUT_PRESENT,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	srv_to_rtv(ID3D12Resource*			   p_resource,
			   D3D12_BARRIER_SYNC		   sync_before_srv,
			   D3D12_TEXTURE_BARRIER_FLAGS flag			   = {},
			   bool						   is_direct_queue = true) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = sync_before_srv,
			.SyncAfter	  = D3D12_BARRIER_SYNC_RENDER_TARGET,
			.AccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.AccessAfter  = D3D12_BARRIER_ACCESS_RENDER_TARGET,
			.LayoutBefore = is_direct_queue
							  ? D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE
							  : D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE,
			.LayoutAfter  = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = flag
		};
	}

	FORCE_INLINE decltype(auto)
	srv_to_dsv_read(ID3D12Resource*				p_resource,
					D3D12_BARRIER_SYNC			sync_before_srv,
					D3D12_TEXTURE_BARRIER_FLAGS flag			= {},
					bool						is_direct_queue = true) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = sync_before_srv,
			.SyncAfter	  = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
			.AccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.AccessAfter  = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
			.LayoutBefore = is_direct_queue
							  ? D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE
							  : D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE,
			.LayoutAfter  = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	srv_to_dsv_write(ID3D12Resource*			 p_resource,
					 D3D12_BARRIER_SYNC			 sync_before_srv,
					 D3D12_TEXTURE_BARRIER_FLAGS flag			 = {},
					 bool						 is_direct_queue = true) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = sync_before_srv,
			.SyncAfter	  = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
			.AccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.AccessAfter  = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
			.LayoutBefore = is_direct_queue
							  ? D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE
							  : D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE,
			.LayoutAfter  = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	rtv_to_srv(ID3D12Resource*			   p_resource,
			   D3D12_BARRIER_SYNC		   sync_after_srv,
			   D3D12_TEXTURE_BARRIER_FLAGS flag			   = {},
			   bool						   is_direct_queue = true) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = D3D12_BARRIER_SYNC_RENDER_TARGET,
			.SyncAfter	  = sync_after_srv,
			.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET,
			.AccessAfter  = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
			.LayoutAfter  = is_direct_queue
							  ? D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE
							  : D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	dsv_write_to_srv(ID3D12Resource*			 p_resource,
					 D3D12_BARRIER_SYNC			 sync_after_srv,
					 D3D12_TEXTURE_BARRIER_FLAGS flag			 = {},
					 bool						 is_direct_queue = true) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
			.SyncAfter	  = sync_after_srv,
			.AccessBefore = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
			.AccessAfter  = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.LayoutBefore = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,
			.LayoutAfter  = is_direct_queue
							  ? D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE
							  : D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	dsv_read_to_dsv_write(ID3D12Resource* p_resource, D3D12_TEXTURE_BARRIER_FLAGS flag = {}) noexcept
	{
		return D3D12_TEXTURE_BARRIER{
			.SyncBefore	  = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
			.SyncAfter	  = D3D12_BARRIER_SYNC_DEPTH_STENCIL,
			.AccessBefore = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,
			.AccessAfter  = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
			.LayoutBefore = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ,
			.LayoutAfter  = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,
			.pResource	  = p_resource,
			.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE{ .IndexOrFirstMipLevel = 0xFFFFFFFF },
			.Flags		  = {}
		};
	}

	FORCE_INLINE decltype(auto)
	buf_uav_to_srv(ID3D12Resource*	  p_resource,
				   D3D12_BARRIER_SYNC sync_before_uav,
				   D3D12_BARRIER_SYNC sync_after_srv) noexcept
	{
		return D3D12_BUFFER_BARRIER{
			.SyncBefore	  = sync_before_uav,
			.SyncAfter	  = sync_after_srv,
			.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
			.AccessAfter  = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.pResource	  = p_resource,
			.Offset		  = 0,
			.Size		  = UINT64_MAX,
		};
	}

	FORCE_INLINE decltype(auto)
	buf_uav_to_srv(ID3D12Resource*	  p_resource,
				   D3D12_BARRIER_SYNC sync_after_srv) noexcept
	{
		return D3D12_BUFFER_BARRIER{
			.SyncBefore	  = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
			.SyncAfter	  = sync_after_srv,
			.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
			.AccessAfter  = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.pResource	  = p_resource,
			.Offset		  = 0,
			.Size		  = UINT64_MAX,
		};
	}

	FORCE_INLINE decltype(auto)
	buf_uav_to_uav(ID3D12Resource*	  p_resource,
				   D3D12_BARRIER_SYNC sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
				   D3D12_BARRIER_SYNC sync_after  = D3D12_BARRIER_SYNC_COMPUTE_SHADING) noexcept
	{
		return D3D12_BUFFER_BARRIER{
			.SyncBefore	  = sync_before,
			.SyncAfter	  = sync_after,
			.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
			.AccessAfter  = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
			.pResource	  = p_resource,
			.Offset		  = 0,
			.Size		  = UINT64_MAX,
		};
	}

	FORCE_INLINE decltype(auto)
	buf_srv_to_uav(ID3D12Resource*	  p_resource,
				   D3D12_BARRIER_SYNC sync_before,
				   D3D12_BARRIER_SYNC sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING) noexcept
	{
		return D3D12_BUFFER_BARRIER{
			.SyncBefore	  = sync_before,
			.SyncAfter	  = sync_after,
			.AccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
			.AccessAfter  = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
			.pResource	  = p_resource,
			.Offset		  = 0,
			.Size		  = UINT64_MAX,
		};
	}

	inline constexpr auto as_frame_start = AGE_LAMBDA((auto&& barrier), {
		barrier.SyncBefore	 = D3D12_BARRIER_SYNC_NONE;
		barrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		return barrier;
	});

	inline constexpr auto as_split_begin = AGE_LAMBDA((auto&& barrier), {
		barrier.SyncAfter = D3D12_BARRIER_SYNC_SPLIT;
		return barrier;
	});

	inline constexpr auto as_split_end = AGE_LAMBDA((auto&& barrier), {
		barrier.SyncBefore = D3D12_BARRIER_SYNC_SPLIT;
		return barrier;
	});

	template <typename t_barrier>
	FORCE_INLINE decltype(auto)
	operator|(t_barrier barrier, auto&& modifier) noexcept
		requires(meta::variadic_contains_v<BARE_OF(modifier),
										   BARE_OF(as_frame_start),
										   BARE_OF(as_split_begin),
										   BARE_OF(as_split_end)>)
	{
		return modifier(barrier);
	}
};	  // namespace age::graphics::barrier

namespace age::graphics
{
	FORCE_INLINE void
	apply_barriers(t_cmd_list& cmd_list, auto&&... barrier) noexcept
	{
		c_auto global_barrier_arr  = meta::make_filtered_array<D3D12_GLOBAL_BARRIER>(FWD(barrier)...);
		c_auto texture_barrier_arr = meta::make_filtered_array<D3D12_TEXTURE_BARRIER>(FWD(barrier)...);
		c_auto buffer_barrier_arr  = meta::make_filtered_array<D3D12_BUFFER_BARRIER>(FWD(barrier)...);

		constexpr auto group_count = (global_barrier_arr.size() > 0)
								   + (texture_barrier_arr.size() > 0)
								   + (buffer_barrier_arr.size() > 0);

		if constexpr (group_count > 0)
		{
			D3D12_BARRIER_GROUP groups[group_count];
			uint32				i = 0;

			if constexpr (global_barrier_arr.size() > 0)
			{
				groups[i++] = { D3D12_BARRIER_TYPE_GLOBAL, (uint32)global_barrier_arr.size(), { .pGlobalBarriers = global_barrier_arr.data() } };
			}
			if constexpr (texture_barrier_arr.size() > 0)
			{
				groups[i++] = { D3D12_BARRIER_TYPE_TEXTURE, (uint32)texture_barrier_arr.size(), { .pTextureBarriers = texture_barrier_arr.data() } };
			}
			if constexpr (buffer_barrier_arr.size() > 0)
			{
				groups[i++] = { D3D12_BARRIER_TYPE_BUFFER, (uint32)buffer_barrier_arr.size(), { .pBufferBarriers = buffer_barrier_arr.data() } };
			}

			cmd_list.Barrier(group_count, groups);
		}
	}
}	 // namespace age::graphics
