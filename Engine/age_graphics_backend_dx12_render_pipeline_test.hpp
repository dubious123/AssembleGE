#pragma once
#include "age.hpp"

namespace age::graphics::g
{
	inline auto h_geometry_stage_default_pso	  = pso::handle{};
	inline auto h_geometry_stage_default_root_sig = root_signature::handle{};

	inline auto h_fx_present_stage_default_pso		= pso::handle{};
	inline auto h_fx_present_stage_default_root_sig = root_signature::handle{};
}	 // namespace age::graphics::g

namespace age::graphics::stage
{
	struct my_stage
	{
		AGE_DEFINE_LOCAL_RESOURCE_VIEW(
			main_buffer_view,
			AGE_RESOURCE_VIEW_VALIDATE(
				AGE_VALIDATE_DIMENSION(D3D12_RESOURCE_DIMENSION_TEXTURE2D)),
			AGE_DESC_HANDLE_MEMBER_RTV(
				h_rtv_desc,
				D3D12_RENDER_TARGET_VIEW_DESC{
					.Format		   = DXGI_FORMAT_R16G16B16A16_FLOAT,
					.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
					.Texture2D	   = { .MipSlice = 0, .PlaneSlice = 0 } }),

			AGE_DESC_HANDLE_MEMBER_SRV(
				h_srv_desc,
				D3D12_SHADER_RESOURCE_VIEW_DESC{
					.Format					 = DXGI_FORMAT_R16G16B16A16_FLOAT,
					.ViewDimension			 = D3D12_SRV_DIMENSION_TEXTURE2D,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Texture2D				 = { .MostDetailedMip	  = 0,
												 .MipLevels			  = 1,
												 .PlaneSlice		  = 0,
												 .ResourceMinLODClamp = 0.f } }));

		AGE_DEFINE_LOCAL_RESOURCE_VIEW(
			depth_buffer_view,
			AGE_RESOURCE_VIEW_VALIDATE(
				AGE_VALIDATE_DIMENSION(D3D12_RESOURCE_DIMENSION_TEXTURE2D)),
			AGE_DESC_HANDLE_MEMBER_DSV(
				h_dsv_desc,
				D3D12_DEPTH_STENCIL_VIEW_DESC{
					.Format		   = DXGI_FORMAT_D32_FLOAT,
					.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
					.Flags		   = D3D12_DSV_FLAG_NONE,
					.Texture2D	   = { .MipSlice = 0 } }));

		AGE_RESOURCE_FLOW_PHASE(
			phase_geo,
			AGE_RESOURCE_BARRIER_ARR(
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(main_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						.StateAfter	 = D3D12_RESOURCE_STATE_RENDER_TARGET,
					},
				},
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(depth_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_DEPTH_READ,
						.StateAfter	 = D3D12_RESOURCE_STATE_DEPTH_WRITE,
					},
				}),
			AGE_RENDER_PASS_RT_DESC_ARR({
				.cpuDescriptor	 = main_buffer_view.h_rtv_desc.h_cpu,
				.BeginningAccess = {
					.Type  = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
					.Clear = {
						.ClearValue = {
							.Format = main_buffer_view.h_rtv_desc_view_desc().Format,
							.Color	= { 0.5f, 0.5f, 0.5f, 1.0f },
						},
					},
				},
				.EndingAccess = { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE },
			}),
			AGE_RENDER_PASS_DS_DESC({
				.cpuDescriptor		  = depth_buffer_view.h_dsv_desc.h_cpu,
				.DepthBeginningAccess = {
					.Type  = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR,
					.Clear = {
						.ClearValue = {
							.Format		  = depth_buffer_view.h_dsv_desc_view_desc().Format,
							.DepthStencil = { .Depth = 1.f },
						},
					},
				},
				.StencilBeginningAccess = { .Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS },
				.DepthEndingAccess		= { .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD },
				.StencilEndingAccess{ .Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS },
			}));

		AGE_RESOURCE_FLOW_PHASE(
			phase_fx,
			AGE_RESOURCE_BARRIER_ARR(
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(main_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
						.StateAfter	 = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					},
				},
				{
					.Type		= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags		= D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition = {
						.pResource	 = const_cast<ID3D12Resource*>(depth_buffer_view.p_resource),
						.Subresource = 0,
						.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE,
						.StateAfter	 = D3D12_RESOURCE_STATE_DEPTH_READ,
					},
				}),
			AGE_RENDER_PASS_RT_DESC_ARR(), AGE_RENDER_PASS_DS_DESC());

		ID3D12RootSignature* p_root_sig = nullptr;
		ID3D12PipelineState* p_pso		= nullptr;

		void
		init() noexcept;

		void
		bind(graphics::root_signature::handle,
			 graphics::pso::handle,
			 graphics::resource_handle h_main_buffer,
			 graphics::resource_handle h_depth_buffer) noexcept;

		void
		execute(t_cmd_list& cmd_list, render_surface& rs) noexcept;

		void
		deinit() noexcept;
	};

	struct my_pipeline
	{
		my_stage stage{};

		extent_2d<uint16> extent{ .width = 100, .height = 100 };

		resource_handle h_main_buffer{};
		resource_handle h_depth_buffer{};

		void
		init() noexcept
		{
			stage.init();
			this->create_buffers();
		}

		void
		execute(t_cmd_list& cmd_list, render_surface& rs) noexcept;

		void
		deinit() noexcept
		{
			stage.deinit();
		}

	  private:
		void
		create_buffers() noexcept
		{
			h_main_buffer = resource::create_resource(
				{ .d3d12_desc{
					  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					  .Alignment		= 0,
					  .Width			= extent.width,
					  .Height			= extent.height,
					  .DepthOrArraySize = 1,
					  .MipLevels		= 1,
					  .Format			= DXGI_FORMAT_R16G16B16A16_FLOAT,
					  .SampleDesc		= { 1, 0 },
					  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
					  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET },
				  .clear_value{
					  .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
					  .Color  = { 0.5f, 0.5f, 0.5f, 1.0f } },
				  .initial_state	= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				  .heap_memory_kind = resource::memory_kind::gpu_only,
				  .has_clear_value	= true });

			h_depth_buffer = resource::create_resource(
				{ .d3d12_desc{
					  .Dimension		= D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					  .Alignment		= 0,
					  .Width			= extent.width,
					  .Height			= extent.height,
					  .DepthOrArraySize = 1,
					  .MipLevels		= 1,
					  .Format			= DXGI_FORMAT_D32_FLOAT,
					  .SampleDesc		= { 1, 0 },
					  .Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN,
					  .Flags			= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL },
				  .clear_value{
					  .Format		= DXGI_FORMAT_D32_FLOAT,
					  .DepthStencil = { .Depth = 1.f, .Stencil = 0 } },
				  .initial_state	= D3D12_RESOURCE_STATE_DEPTH_READ,
				  .heap_memory_kind = resource::memory_kind::gpu_only,
				  .has_clear_value	= true });
		}

		void
		resize(const age::extent_2d<uint16>& new_extent) noexcept
		{
			extent = new_extent;

			resource::release_resource(h_main_buffer);
			resource::release_resource(h_depth_buffer);

			this->create_buffers();
		}
	};
}	 // namespace age::graphics::stage

namespace age::graphics::g
{
	inline auto test_pipeline = stage::my_pipeline{};
}