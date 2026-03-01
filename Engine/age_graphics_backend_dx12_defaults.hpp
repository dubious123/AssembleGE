#pragma once
#include "age.hpp"

// resource desc

namespace age::graphics::defaults
{
	namespace heap_properties
	{
		constexpr FORCE_INLINE decltype(auto)
		committed_heap(age::graphics::resource::e::memory_kind kind) noexcept
		{
			return D3D12_HEAP_PROPERTIES{
				.Type				  = static_cast<D3D12_HEAP_TYPE>(kind),
				.CPUPageProperty	  = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask	  = 1,
				.VisibleNodeMask	  = 1
			};
		}
	}	 // namespace heap_properties

	namespace resource_desc
	{
		FORCE_INLINE decltype(auto)
		buffer(uint64 byte_size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) noexcept
		{
			return D3D12_RESOURCE_DESC{
				/*D3D12_RESOURCE_DIMENSION		*/ .Dimension		 = D3D12_RESOURCE_DIMENSION_BUFFER,
				/*UINT64						*/ .Alignment		 = 0,
				/*UINT64						*/ .Width			 = byte_size,
				/*UINT							*/ .Height			 = 1,
				/*UINT16						*/ .DepthOrArraySize = 1,
				/*UINT16						*/ .MipLevels		 = 1,
				/*DXGI_FORMAT					*/ .Format			 = DXGI_FORMAT_UNKNOWN,
				/*DXGI_SAMPLE_DESC				*/ .SampleDesc		 = { .Count = 1, .Quality = 0 },
				/*D3D12_TEXTURE_LAYOUT			*/ .Layout			 = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				/*D3D12_RESOURCE_FLAGS			*/ .Flags			 = flags
			};
		}

		FORCE_INLINE decltype(auto)
		buffer_uav(uint64 byte_size) noexcept
		{
			return buffer(byte_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		}

		FORCE_INLINE decltype(auto)
		texture_2d_array(uint32				  width,
						 uint32				  height,
						 DXGI_FORMAT		  format,
						 uint16				  array_size,
						 D3D12_RESOURCE_FLAGS flags			 = D3D12_RESOURCE_FLAG_NONE,
						 uint16				  mip_levels	 = 1,
						 uint16				  sample_count	 = 1,
						 uint16				  sample_quality = 0) noexcept
		{
			AGE_ASSERT(sample_count == 1 or mip_levels == 1);
			return D3D12_RESOURCE_DESC{
				/*D3D12_RESOURCE_DIMENSION		*/ .Dimension		 = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				/*UINT64						*/ .Alignment		 = 0,
				/*UINT64						*/ .Width			 = width,
				/*UINT							*/ .Height			 = height,
				/*UINT16						*/ .DepthOrArraySize = array_size,
				/*UINT16						*/ .MipLevels		 = mip_levels,
				/*DXGI_FORMAT					*/ .Format			 = format,
				/*DXGI_SAMPLE_DESC				*/ .SampleDesc		 = { .Count = sample_count, .Quality = sample_quality },
				/*D3D12_TEXTURE_LAYOUT			*/ .Layout			 = D3D12_TEXTURE_LAYOUT_UNKNOWN,
				/*D3D12_RESOURCE_FLAGS			*/ .Flags			 = flags
			};
		}

		FORCE_INLINE decltype(auto)
		texture_2d(uint32				width,
				   uint32				height,
				   DXGI_FORMAT			format,
				   D3D12_RESOURCE_FLAGS flags		   = D3D12_RESOURCE_FLAG_NONE,
				   uint16				mip_levels	   = 1,
				   uint16				sample_count   = 1,
				   uint16				sample_quality = 0) noexcept
		{
			return texture_2d_array(width,
									height,
									format,
									1,
									flags,
									mip_levels,
									sample_count,
									sample_quality);
		}

		FORCE_INLINE decltype(auto)
		texture_rt_2d(uint32			   width,
					  uint32			   height,
					  D3D12_RESOURCE_FLAGS extra_flags	  = D3D12_RESOURCE_FLAG_NONE,
					  DXGI_FORMAT		   format		  = DXGI_FORMAT_R16G16B16A16_FLOAT,
					  uint16			   array_size	  = 1,
					  uint16			   mip_levels	  = 1,
					  uint16			   sample_count	  = 1,
					  uint16			   sample_quality = 0) noexcept
		{
			return texture_2d_array(width,
									height,
									format,
									array_size,
									D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | extra_flags,
									mip_levels,
									sample_count,
									sample_quality);
		}

		FORCE_INLINE decltype(auto)
		texture_ds_2d(uint32			   width,
					  uint32			   height,
					  D3D12_RESOURCE_FLAGS extra_flags	  = D3D12_RESOURCE_FLAG_NONE,
					  DXGI_FORMAT		   format		  = DXGI_FORMAT_D32_FLOAT,
					  uint16			   array_size	  = 1,
					  uint16			   mip_levels	  = 1,
					  uint16			   sample_count	  = 1,
					  uint16			   sample_quality = 0) noexcept
		{
			return texture_2d_array(width,
									height,
									format,
									array_size,
									D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | extra_flags,
									mip_levels,
									sample_count,
									sample_quality);
		}
	}	 // namespace resource_desc

}	 // namespace age::graphics::defaults

// resource view descs
namespace age::graphics::defaults
{
	namespace rtv_view_desc
	{
		// main buffer format for hdr rendering
		inline constexpr auto hdr_rgba16_2d = D3D12_RENDER_TARGET_VIEW_DESC{
			.Format		   = DXGI_FORMAT_R16G16B16A16_FLOAT,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D	   = { .MipSlice = 0, .PlaneSlice = 0 },
		};

		// back buffer format for hdr rendering (for swap chain buffers with hdr support)
		inline constexpr auto hdr10_2d = D3D12_RENDER_TARGET_VIEW_DESC{
			.Format		   = DXGI_FORMAT_R10G10B10A2_UNORM,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D	   = { .MipSlice = 0, .PlaneSlice = 0 },
		};

		// back buffer format for sRGB rendering (for swap chain buffers with sRGB support)
		inline constexpr auto srgb_2d = D3D12_RENDER_TARGET_VIEW_DESC{
			.Format		   = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
			.Texture2D	   = { .MipSlice = 0, .PlaneSlice = 0 },
		};
	}	 // namespace rtv_view_desc

	namespace dsv_view_desc
	{
		inline constexpr auto d32_float_2d = D3D12_DEPTH_STENCIL_VIEW_DESC{
			.Format		   = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags		   = D3D12_DSV_FLAG_NONE,
			.Texture2D	   = { .MipSlice = 0 }
		};

		inline constexpr auto d32_float_2d_readonly = D3D12_DEPTH_STENCIL_VIEW_DESC{
			.Format		   = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags		   = D3D12_DSV_FLAG_READ_ONLY_DEPTH,
			.Texture2D	   = { .MipSlice = 0 }
		};
	}	 // namespace dsv_view_desc

	namespace srv_view_desc
	{
		FORCE_INLINE decltype(auto)
		tex2d(DXGI_FORMAT format, uint32 mip_levels = 1) noexcept
		{
			AGE_ASSERT(mip_levels > 0);
			return D3D12_SHADER_RESOURCE_VIEW_DESC{
				.Format					 = format,
				.ViewDimension			 = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D				 = D3D12_TEX2D_SRV{
					.MostDetailedMip	 = 0,
					.MipLevels			 = mip_levels,
					.PlaneSlice			 = 0,
					.ResourceMinLODClamp = 0.0f }
			};
		}
	}	 // namespace srv_view_desc

	namespace uav_view_desc
	{
		FORCE_INLINE decltype(auto)
		byte_address_buffer(uint64 byte_size) noexcept
		{
			return D3D12_UNORDERED_ACCESS_VIEW_DESC{
				.Format		   = DXGI_FORMAT_R32_TYPELESS,
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
				.Buffer		   = {
					.FirstElement		  = 0,
					.NumElements		  = static_cast<uint32>(byte_size / 4),
					.StructureByteStride  = 0,
					.CounterOffsetInBytes = 0,
					.Flags				  = D3D12_BUFFER_UAV_FLAG_RAW }
			};
		}

		FORCE_INLINE decltype(auto)
		structured_buffer(uint32 element_count, uint32 byte_size) noexcept
		{
			return D3D12_UNORDERED_ACCESS_VIEW_DESC{
				.Format		   = DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
				.Buffer		   = {
					.FirstElement		  = 0,
					.NumElements		  = element_count,
					.StructureByteStride  = byte_size,
					.CounterOffsetInBytes = 0,
					.Flags				  = D3D12_BUFFER_UAV_FLAG_NONE }
			};
		}
	}	 // namespace uav_view_desc
}	 // namespace age::graphics::defaults

// pss subobjects
namespace age::graphics::defaults
{
	namespace rasterizer_desc
	{
		inline constexpr auto no_cull = D3D12_RASTERIZER_DESC{
			D3D12_FILL_MODE_SOLID,						  // FillMode
			D3D12_CULL_MODE_NONE,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};

		inline constexpr auto backface_cull = D3D12_RASTERIZER_DESC{
			D3D12_FILL_MODE_SOLID,						  // FillMode
			D3D12_CULL_MODE_BACK,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};

		inline constexpr auto frontface_cull = D3D12_RASTERIZER_DESC{
			D3D12_FILL_MODE_SOLID,						  // FillMode
			D3D12_CULL_MODE_FRONT,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};

		inline constexpr auto wireframe = D3D12_RASTERIZER_DESC{
			D3D12_FILL_MODE_WIREFRAME,					  // FillMode
			D3D12_CULL_MODE_NONE,						  // CullMode
			0,											  // FrontCounterClockwise
			0,											  // DepthBias
			0,											  // DepthBiasClamp
			0,											  // SlopeScaledDepthBias
			1,											  // DepthClipEnable
			1,											  // MultisampleEnable
			0,											  // AntialiasedLineEnable
			0,											  // ForcedSampleCount
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,	  // ConservativeRaster
		};
	}	 // namespace rasterizer_desc

	namespace depth_stencil_desc1
	{
		inline constexpr auto disabled = D3D12_DEPTH_STENCIL_DESC1{
			0,										// DepthEnable
			D3D12_DEPTH_WRITE_MASK_ZERO,			// DepthWriteMask
			D3D12_COMPARISON_FUNC_LESS_EQUAL,		// DepthFunc
			0,										// StencilEnable
			0,										// StencilReadMask
			0,										// StencilWriteMask
			{},										// FrontFace
			{},										// BackFace
			0										// DepthBoundsTestEnable
		};

		inline constexpr auto depth_only = D3D12_DEPTH_STENCIL_DESC1{
			TRUE,									// DepthEnable
			D3D12_DEPTH_WRITE_MASK_ALL,				// DepthWriteMask
			D3D12_COMPARISON_FUNC_LESS_EQUAL,		// DepthFunc
			FALSE,									// StencilEnable
			D3D12_DEFAULT_STENCIL_READ_MASK,		// StencilReadMask
			D3D12_DEFAULT_STENCIL_WRITE_MASK,		// StencilWriteMask
			{},										// FrontFace
			{},										// BackFace
			FALSE									// DepthBoundsTestEnable
		};


		inline constexpr auto depth_readonly = D3D12_DEPTH_STENCIL_DESC1{
			TRUE,									// DepthEnable
			D3D12_DEPTH_WRITE_MASK_ZERO,			// DepthWriteMask
			D3D12_COMPARISON_FUNC_LESS_EQUAL,		// DepthFunc
			FALSE,									// StencilEnable
			D3D12_DEFAULT_STENCIL_READ_MASK,		// StencilReadMask
			D3D12_DEFAULT_STENCIL_WRITE_MASK,		// StencilWriteMask
			{},										// FrontFace
			{},										// BackFace
			FALSE									// DepthBoundsTestEnable
		};

		inline constexpr auto depth_only_reversed = D3D12_DEPTH_STENCIL_DESC1{
			TRUE,									// DepthEnable
			D3D12_DEPTH_WRITE_MASK_ALL,				// DepthWriteMask
			D3D12_COMPARISON_FUNC_GREATER_EQUAL,	// DepthFunc
			FALSE,									// StencilEnable
			D3D12_DEFAULT_STENCIL_READ_MASK,		// StencilReadMask
			D3D12_DEFAULT_STENCIL_WRITE_MASK,		// StencilWriteMask
			{},										// FrontFace
			{},										// BackFace
			FALSE									// DepthBoundsTestEnable
		};


		inline constexpr auto depth_readonly_reversed = D3D12_DEPTH_STENCIL_DESC1{
			TRUE,									// DepthEnable
			D3D12_DEPTH_WRITE_MASK_ZERO,			// DepthWriteMask
			D3D12_COMPARISON_FUNC_GREATER_EQUAL,	// DepthFunc
			FALSE,									// StencilEnable
			D3D12_DEFAULT_STENCIL_READ_MASK,		// StencilReadMask
			D3D12_DEFAULT_STENCIL_WRITE_MASK,		// StencilWriteMask
			{},										// FrontFace
			{},										// BackFace
			FALSE									// DepthBoundsTestEnable
		};

		inline constexpr auto depth_equal_readonly_reversed = D3D12_DEPTH_STENCIL_DESC1{
			TRUE,									// DepthEnable
			D3D12_DEPTH_WRITE_MASK_ZERO,			// DepthWriteMask
			D3D12_COMPARISON_FUNC_EQUAL,			// DepthFunc
			FALSE,									// StencilEnable
			D3D12_DEFAULT_STENCIL_READ_MASK,		// StencilReadMask
			D3D12_DEFAULT_STENCIL_WRITE_MASK,		// StencilWriteMask
			{},										// FrontFace
			{},										// BackFace
			FALSE									// DepthBoundsTestEnable
		};
	}	 // namespace depth_stencil_desc1

	namespace blend_desc
	{
		inline constexpr auto opaque = D3D12_BLEND_DESC{
			.AlphaToCoverageEnable	= FALSE,							  // BOOL
			.IndependentBlendEnable = FALSE,							  // BOOL
			.RenderTarget			= { D3D12_RENDER_TARGET_BLEND_DESC{
				.BlendEnable		   = FALSE,							  // BOOL
				.LogicOpEnable		   = FALSE,							  // BOOL
				.SrcBlend			   = D3D12_BLEND_ONE,				  // D3D12_BLEND
				.DestBlend			   = D3D12_BLEND_ZERO,				  // D3D12_BLEND
				.BlendOp			   = D3D12_BLEND_OP_ADD,			  // D3D12_BLEND_OP
				.SrcBlendAlpha		   = D3D12_BLEND_ONE,				  // D3D12_BLEND
				.DestBlendAlpha		   = D3D12_BLEND_ZERO,				  // D3D12_BLEND
				.BlendOpAlpha		   = D3D12_BLEND_OP_ADD,			  // D3D12_BLEND_OP
				.LogicOp			   = D3D12_LOGIC_OP_NOOP,			  // D3D12_LOGIC_OP
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,	  // UINT8
			} },
		};
	}
}	 // namespace age::graphics::defaults

// render pass descs
namespace age::graphics::defaults
{
	namespace render_pass_rtv_desc
	{
		FORCE_INLINE decltype(auto)
		load_preserve(rtv_desc_handle h_rtv_desc) noexcept
		{
			D3D12_RENDER_PASS_RENDER_TARGET_DESC desc = {
				.cpuDescriptor	 = h_rtv_desc.h_cpu,
				.BeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE },
				.EndingAccess	 = { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE }
			};

			return desc;
		}

		FORCE_INLINE decltype(auto)
		overwrite_preserve(rtv_desc_handle h_rtv_desc) noexcept
		{
			return D3D12_RENDER_PASS_RENDER_TARGET_DESC{
				.cpuDescriptor	 = h_rtv_desc.h_cpu,
				.BeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD },
				.EndingAccess	 = { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE }
			};
		}

		FORCE_INLINE decltype(auto)
		clear_preserve(rtv_desc_handle h_rtv_desc, const float* p_clear_color) noexcept
		{
			D3D12_RENDER_PASS_RENDER_TARGET_DESC desc = {
				.cpuDescriptor	 = h_rtv_desc.h_cpu,
				.BeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR },
				.EndingAccess	 = { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE }
			};
			if (p_clear_color)
			{
				std::memcpy(desc.BeginningAccess.Clear.ClearValue.Color, p_clear_color, sizeof(float) * 4);
			}
			return desc;
		}
	}	 // namespace render_pass_rtv_desc

	namespace render_pass_ds_desc
	{
		// stencil no access
		FORCE_INLINE decltype(auto)
		depth_load_preserve(dsv_desc_handle h_dsv_desc) noexcept
		{
			return D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
				.cpuDescriptor		  = h_dsv_desc.h_cpu,
				.DepthBeginningAccess = {
					.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE },
				.StencilBeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS },
				.DepthEndingAccess		= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE },
				.StencilEndingAccess	= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS }
			};
		}

		FORCE_INLINE decltype(auto)
		depth_clear_preserve(dsv_desc_handle h_dsv_desc, float depth_val = 1.0f) noexcept
		{
			return D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
				.cpuDescriptor		  = h_dsv_desc.h_cpu,
				.DepthBeginningAccess = {
					.Type  = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
					.Clear = { .ClearValue = { .Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = { .Depth = depth_val } } } },
				.StencilBeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS },
				.DepthEndingAccess		= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE },
				.StencilEndingAccess	= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS }
			};
		}

		FORCE_INLINE decltype(auto)
		depth_clear_discard(dsv_desc_handle h_dsv_desc, float depth_val = 1.0f) noexcept
		{
			auto desc					= depth_clear_preserve(h_dsv_desc, depth_val);
			desc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
			return desc;
		}

		FORCE_INLINE decltype(auto)
		depth_no_access(dsv_desc_handle h_dsv_desc) noexcept
		{
			return D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
				.cpuDescriptor			= h_dsv_desc.h_cpu,
				.DepthBeginningAccess	= { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS },
				.StencilBeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS },
				.DepthEndingAccess		= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS },
				.StencilEndingAccess	= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS }
			};
		}

		FORCE_INLINE decltype(auto)
		depth_stencil_clear_preserve(
			dsv_desc_handle h_dsv_desc,
			float			depth_val	= 1.0f,
			uint8_t			stencil_val = 0) noexcept
		{
			return D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{
				.cpuDescriptor		  = h_dsv_desc.h_cpu,
				.DepthBeginningAccess = {
					.Type  = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
					.Clear = { .ClearValue = { .Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = { .Depth = depth_val } } } },
				.StencilBeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, .Clear = { .ClearValue = { .Format = DXGI_FORMAT_D24_UNORM_S8_UINT, .DepthStencil = { .Stencil = stencil_val } } } },
				.DepthEndingAccess		= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE },
				.StencilEndingAccess	= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE }
			};
		}

		FORCE_INLINE decltype(auto)
		depth_clear_stencil_preserve(dsv_desc_handle h_dsv_desc, float depth_val = 1.0f) noexcept
		{
			auto desc						 = depth_clear_preserve(h_dsv_desc, depth_val);
			desc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
			desc.StencilEndingAccess.Type	 = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
			return desc;
		}
	}	 // namespace render_pass_ds_desc
}	 // namespace age::graphics::defaults

// root signature static sampler desc
namespace age::graphics::defaults::static_sampler_desc
{
	FORCE_INLINE decltype(auto)
	linear_clamp(uint32 register_index, uint32 register_space, D3D12_SHADER_VISIBILITY shader_visibility, D3D12_SAMPLER_FLAGS sampler_flags) noexcept
	{
		return D3D12_STATIC_SAMPLER_DESC1{
			/*D3D12_FILTER					*/ .Filter			 = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			/*D3D12_TEXTURE_ADDRESS_MODE	*/ .AddressU		 = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			/*D3D12_TEXTURE_ADDRESS_MODE	*/ .AddressV		 = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			/*D3D12_TEXTURE_ADDRESS_MODE	*/ .AddressW		 = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			/*FLOAT							*/ .MipLODBias		 = 0.0f,
			/*UINT							*/ .MaxAnisotropy	 = 1,
			/*D3D12_COMPARISON_FUNC			*/ .ComparisonFunc	 = D3D12_COMPARISON_FUNC_NEVER,
			/*D3D12_STATIC_BORDER_COLOR		*/ .BorderColor		 = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
			/*FLOAT							*/ .MinLOD			 = 0.f,
			/*FLOAT							*/ .MaxLOD			 = D3D12_FLOAT32_MAX,
			/*UINT							*/ .ShaderRegister	 = register_index,
			/*UINT							*/ .RegisterSpace	 = register_space,
			/*D3D12_SHADER_VISIBILITY		*/ .ShaderVisibility = shader_visibility,
			/*D3D12_SAMPLER_FLAGS			*/ .Flags			 = sampler_flags
		};
	}
}	 // namespace age::graphics::defaults::static_sampler_desc