#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
namespace age::graphics
{
	void
	render_surface::init(age::platform::window_handle w_handle) noexcept
	{
		this->h_window = w_handle;

		{
			auto allow_tearing = BOOL{ graphics::is_tearing_allowed() };

			AGE_HR_CHECK(g::p_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)));
			present_flags = allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : UINT{ 0 };


			DXGI_FORMAT dxgi_format{};
			{
				switch (global::get<interface>().display_color_space())
				{
				case color_space::srgb:
					dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					rtv_format	= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					break;
				case color_space::hdr:
					dxgi_format = DXGI_FORMAT_R10G10B10A2_UNORM;
					rtv_format	= DXGI_FORMAT_R10G10B10A2_UNORM;
					break;
				default:
					std::unreachable();
				}
			}

			auto swap_chain_desc = DXGI_SWAP_CHAIN_DESC1{
				/*UINT					*/ .Width		= platform::get_client_width(w_handle),
				/*UINT					*/ .Height		= platform::get_client_height(w_handle),
				/*DXGI_FORMAT			*/ .Format		= dxgi_format,
				/*BOOL					*/ .Stereo		= false,
				/*DXGI_SAMPLE_DESC		*/ .SampleDesc	= { .Count = 1, .Quality = 0 },
				/*DXGI_USAGE			*/ .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				/*UINT					*/ .BufferCount = g::frame_buffer_count,
				/*DXGI_SCALING			*/ .Scaling		= DXGI_SCALING_STRETCH,
				/*DXGI_SWAP_EFFECT		*/ .SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD,
				/*DXGI_ALPHA_MODE		*/ .AlphaMode	= DXGI_ALPHA_MODE_UNSPECIFIED,
				/*UINT					*/ .Flags =
					DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
					| (allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : UINT{ 0 }),
			};

			auto full_screen_desc = DXGI_SWAP_CHAIN_FULLSCREEN_DESC{
				/*DXGI_RATIONAL				*/ .RefreshRate		 = DXGI_RATIONAL{ 0, 1 },
				/*DXGI_MODE_SCANLINE_ORDER	*/ .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				/*DXGI_MODE_SCALING			*/ .Scaling			 = DXGI_MODE_SCALING_UNSPECIFIED,
				/*BOOL						*/ .Windowed		 = BOOL{ true },
			};

			auto* p_swap_chain_1 = (IDXGISwapChain1*)nullptr;
			{
				AGE_HR_CHECK(g::p_dxgi_factory->CreateSwapChainForHwnd(
					g::cmd_system_direct.p_cmd_queue,
					platform::get_hwnd(w_handle),
					&swap_chain_desc,
					&full_screen_desc,
					nullptr,
					&p_swap_chain_1));
			}

			AGE_HR_CHECK(g::p_dxgi_factory->MakeWindowAssociation(platform::get_hwnd(w_handle), DXGI_MWA_NO_ALT_ENTER));

			AGE_HR_CHECK(p_swap_chain_1->QueryInterface(IID_PPV_ARGS(&p_swap_chain)));
			p_swap_chain_1->Release();
		}

		AGE_HR_CHECK(p_swap_chain->SetMaximumFrameLatency(3));

		present_waitable_obj = p_swap_chain->GetFrameLatencyWaitableObject();

		AGE_ASSERT(present_waitable_obj != NULL);


		for (uint32 idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			rtv_desc_handle_arr[idx] = g::rtv_desc_pool.pop();
		}

		rebuild_from_swapchain();
	}

	void
	render_surface::resize() noexcept
	{
		for (auto* p_resource : back_buffer_ptr_arr)
		{
			p_resource->Release();
		}

		auto allow_tearing = BOOL{ graphics::is_tearing_allowed() };

		AGE_HR_CHECK(g::p_dxgi_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)));

		p_swap_chain->ResizeBuffers(
			g::frame_buffer_count,
			0,
			0,
			DXGI_FORMAT_UNKNOWN,
			DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
				| (allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : UINT{ 0 }));


		rebuild_from_swapchain();
	}

	void
	render_surface::present() noexcept
	{
		AGE_HR_CHECK(p_swap_chain->Present(0, present_flags));
		back_buffer_idx			  = p_swap_chain->GetCurrentBackBufferIndex();
		last_used_cmd_fence_value = g::current_fence_value;

		std::println("present, fence_value :  {}", g::current_fence_value);
	}

	void
	render_surface::deinit() noexcept
	{
		for (auto idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			g::rtv_desc_pool.push(rtv_desc_handle_arr[idx]);
			back_buffer_ptr_arr[idx]->Release();
		}

		p_swap_chain->Release();

		::CloseHandle(present_waitable_obj);
	}

	void
	render_surface::rebuild_from_swapchain() noexcept
	{
		back_buffer_idx = p_swap_chain->GetCurrentBackBufferIndex();

		for (uint32 idx : std::views::iota(0) | std::views::take(g::frame_buffer_count))
		{
			AGE_HR_CHECK(p_swap_chain->GetBuffer(idx, IID_PPV_ARGS(&back_buffer_ptr_arr[idx])));

			auto rtv_desc = D3D12_RENDER_TARGET_VIEW_DESC{
				/*DXGI_FORMAT			*/ .Format		  = rtv_format,
				/*D3D12_RTV_DIMENSION	*/ .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				/*D3D12_TEX2D_RTV		*/ .Texture2D	  = D3D12_TEX2D_RTV{ .MipSlice = 0, .PlaneSlice = 0 }
			};

			g::p_main_device->CreateRenderTargetView(back_buffer_ptr_arr[idx], &rtv_desc, rtv_desc_handle_arr[idx].h_cpu);
		}

		auto desc = DXGI_SWAP_CHAIN_DESC{};

		AGE_HR_CHECK(p_swap_chain->GetDesc(&desc));
		AGE_ASSERT(desc.BufferDesc.Width == platform::get_client_width(h_window));
		AGE_ASSERT(desc.BufferDesc.Height == platform::get_client_height(h_window));

		default_viewport = D3D12_VIEWPORT{
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width	  = static_cast<float>(desc.BufferDesc.Width),
			.Height	  = static_cast<float>(desc.BufferDesc.Height),
			.MinDepth = 0.f,
			.MaxDepth = 1.f,
		};

		default_scissor_rect = D3D12_RECT{
			.left	= 0l,
			.top	= 0l,
			.right	= static_cast<int32>(desc.BufferDesc.Width),
			.bottom = static_cast<int32>(desc.BufferDesc.Height),
		};
	}
}	 // namespace age::graphics

namespace age::graphics
{
	render_surface_handle
	create_render_surface(platform::window_handle w_handle) noexcept
	{
		auto id = g::render_surface_vec.emplace_back();
		g::render_surface_vec[id].init(w_handle);

		return render_surface_handle{ .id = id };
	}

	render_surface_handle
	find_render_surface(platform::window_handle h_window) noexcept
	{
		for (auto nth : std::views::iota(0ul) | std::views::take(g::render_surface_vec.count()))
		{
			auto  idx = g::render_surface_vec.nth_id(nth);
			auto& rs  = g::render_surface_vec[idx];
			if (rs.h_window == h_window)
			{
				return render_surface_handle{ .id = idx };
			};
		}

		AGE_UNREACHABLE("render_surface not found, h_window : {}, render_surface_vec : {}",
						h_window.id,
						g::render_surface_vec | std::views::transform([](auto& rs) { return rs.h_window.id; }));
	}
}	 // namespace age::graphics

#endif