#include "age_pch.hpp"
#include "age.hpp"
#if defined(AGE_GRAPHICS_BACKEND_DX12)
namespace age::graphics
{
	void
	render_surface::init(age::platform::window_handle h_window) noexcept
	{
		this->h_window = h_window;

		{
			c_auto allow_tearing = graphics::is_tearing_allowed();

			this->present_flags = allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : UINT{ 0 };

			c_auto monitor_data			   = graphics::get_monitor_data(h_window);
			this->min_luminance			   = monitor_data.min_luminance;
			this->max_luminance			   = monitor_data.max_luminance;
			this->max_full_frame_luminance = monitor_data.max_full_frame_luminance;
			this->monitor_name			   = monitor_data.monitor_name;

			if (config::default_hdr_enabled and monitor_data.hdr_enabled)
			{
				// HDR10
				this->rtv_format			 = defaults::rtv_view_desc::hdr10_2d.Format;
				this->color_space			 = e::color_space_kind::hdr10;
				this->preference_color_space = e::color_space_kind::hdr10;
			}
			else
			{
				// sRGB
				this->rtv_format			 = defaults::rtv_view_desc::srgb_2d.Format;
				this->color_space			 = e::color_space_kind::srgb;
				this->preference_color_space = e::color_space_kind::srgb;
			}

			this->swap_chain_flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
								   | (allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : UINT{ 0 });

			c_auto swap_chain_desc = DXGI_SWAP_CHAIN_DESC1{
				/*UINT					*/ .Width		= platform::get_client_width(h_window),
				/*UINT					*/ .Height		= platform::get_client_height(h_window),
				/*DXGI_FORMAT			*/ .Format		= dx12_format(this->color_space),
				/*BOOL					*/ .Stereo		= false,
				/*DXGI_SAMPLE_DESC		*/ .SampleDesc	= { .Count = 1, .Quality = 0 },
				/*DXGI_USAGE			*/ .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				/*UINT					*/ .BufferCount = global::frame_buffer_count,
				/*DXGI_SCALING			*/ .Scaling		= DXGI_SCALING_STRETCH,
				/*DXGI_SWAP_EFFECT		*/ .SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD,
				/*DXGI_ALPHA_MODE		*/ .AlphaMode	= DXGI_ALPHA_MODE_UNSPECIFIED,
				/*UINT					*/ .Flags		= swap_chain_flags,
			};

			c_auto full_screen_desc = DXGI_SWAP_CHAIN_FULLSCREEN_DESC{
				/*DXGI_RATIONAL				*/ .RefreshRate		 = DXGI_RATIONAL{ 0, 1 },
				/*DXGI_MODE_SCANLINE_ORDER	*/ .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
				/*DXGI_MODE_SCALING			*/ .Scaling			 = DXGI_MODE_SCALING_UNSPECIFIED,
				/*BOOL						*/ .Windowed		 = BOOL{ true },
			};

			auto* p_swap_chain_1 = (IDXGISwapChain1*)nullptr;
			{
				AGE_HR_CHECK(g::p_dxgi_factory->CreateSwapChainForHwnd(
					g::queue_ctx[std::to_underlying(e::queue_kind::direct)].p_queue,
					platform::get_hwnd(h_window),
					&swap_chain_desc,
					&full_screen_desc,
					nullptr,
					&p_swap_chain_1));
			}

			AGE_HR_CHECK(g::p_dxgi_factory->MakeWindowAssociation(platform::get_hwnd(h_window), DXGI_MWA_NO_ALT_ENTER));

			AGE_HR_CHECK(p_swap_chain_1->QueryInterface(IID_PPV_ARGS(&this->p_swap_chain)));
			p_swap_chain_1->Release();
		}

		AGE_HR_CHECK(this->p_swap_chain->SetMaximumFrameLatency(global::frame_buffer_count));

		this->present_waitable_obj = this->p_swap_chain->GetFrameLatencyWaitableObject();

		AGE_ASSERT(this->present_waitable_obj != NULL);

		if (c_auto h_res = this->p_swap_chain->SetColorSpace1(dx12_color_space(this->color_space));
			FAILED(h_res))
		{
			AGE_ASSERT(false, "SetColorSpace1 failed (0x{:08X}), one possible reason : race condition, graphics::get_monitor_data -> hdr_enabled, now -> hdr_not_enabled", cast_to<uint32>(h_res));
			std::abort();
		}

		for (c_auto idx : views::loop(global::frame_buffer_count))
		{
			pop_descriptor(AGE_OUT this->rtv_desc_handle_arr[idx]);
		}

		rebuild_from_swapchain();
	}

	void
	render_surface::rebuild(e::color_space_kind new_color_space) noexcept
	{
		for (auto*& p_resource : this->back_buffer_ptr_arr)
		{
			if (p_resource is_not_nullptr)
			{
				p_resource->Release();
				p_resource = nullptr;
			}
		}

		c_auto monitor_data			   = graphics::get_monitor_data(h_window);
		this->min_luminance			   = monitor_data.min_luminance;
		this->max_luminance			   = monitor_data.max_luminance;
		this->max_full_frame_luminance = monitor_data.max_full_frame_luminance;
		this->monitor_name			   = monitor_data.monitor_name;

		this->color_space = (new_color_space == e::color_space_kind::hdr10 and monitor_data.hdr_enabled)
							  ? e::color_space_kind::hdr10
							  : e::color_space_kind::srgb;

		if (this->color_space == e::color_space_kind::srgb)
		{
			this->rtv_format = defaults::rtv_view_desc::srgb_2d.Format;
		}
		else if (this->color_space == e::color_space_kind::hdr10)
		{
			this->rtv_format = defaults::rtv_view_desc::hdr10_2d.Format;
		}
		else
		{
			AGE_UNREACHABLE("invalid color_space_kind : {}", to_idx(this->color_space));
		}

		AGE_HR_CHECK(this->p_swap_chain->ResizeBuffers(
			global::frame_buffer_count,
			0, 0,
			dx12_format(this->color_space),
			this->swap_chain_flags));

		if (c_auto h_res = this->p_swap_chain->SetColorSpace1(dx12_color_space(this->color_space));
			FAILED(h_res))
		{
			if (this->color_space == e::color_space_kind::hdr10)
			{
				rebuild(e::color_space_kind::srgb);
				return;
			}
			else
			{
				AGE_ASSERT(false, "SetColorSpace1 failed (0x{:08X})", cast_to<uint32>(h_res));
				std::abort();
			}
		}

		rebuild_from_swapchain();
	}

	void
	render_surface::resize() noexcept
	{
		rebuild(this->color_space);
	}

	void
	render_surface::present() noexcept
	{
		AGE_HR_CHECK(this->p_swap_chain->Present(0, this->present_flags));
		back_buffer_idx = this->p_swap_chain->GetCurrentBackBufferIndex();

		present_fence_value = command::signal(e::queue_kind::direct);
	}

	void
	render_surface::deinit() noexcept
	{
		for (c_auto idx : views::loop(global::frame_buffer_count))
		{
			push_descriptor(this->rtv_desc_handle_arr[idx]);
			this->back_buffer_ptr_arr[idx]->Release();
		}

		this->p_swap_chain->Release();

		::CloseHandle(this->present_waitable_obj);
	}

	void
	render_surface::rebuild_from_swapchain() noexcept
	{
		this->back_buffer_idx = this->p_swap_chain->GetCurrentBackBufferIndex();

		for (c_auto idx : views::loop(global::frame_buffer_count))
		{
			AGE_HR_CHECK(this->p_swap_chain->GetBuffer(idx, IID_PPV_ARGS(&this->back_buffer_ptr_arr[idx])));

			if (this->color_space == e::color_space_kind::hdr10)
			{
				AGE_ASSERT(defaults::rtv_view_desc::hdr10_2d.Format == this->rtv_format);

				resource::create_view(*this->back_buffer_ptr_arr[idx], this->rtv_desc_handle_arr[idx], defaults::rtv_view_desc::hdr10_2d);
			}
			else if (this->color_space == e::color_space_kind::srgb)
			{
				AGE_ASSERT(defaults::rtv_view_desc::srgb_2d.Format == this->rtv_format);

				resource::create_view(*this->back_buffer_ptr_arr[idx], this->rtv_desc_handle_arr[idx], defaults::rtv_view_desc::srgb_2d);
			}
			else
			{
				AGE_ASSERT(false, "invalid color space");
			}
		}

		{
			auto desc = DXGI_SWAP_CHAIN_DESC{};

			AGE_HR_CHECK(this->p_swap_chain->GetDesc(&desc));
			AGE_ASSERT(desc.BufferDesc.Width == platform::get_client_width(h_window));
			AGE_ASSERT(desc.BufferDesc.Height == platform::get_client_height(h_window));
		}


		this->default_viewport = D3D12_VIEWPORT{
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width	  = static_cast<float>(platform::get_client_width(h_window)),
			.Height	  = static_cast<float>(platform::get_client_height(h_window)),
			.MinDepth = 0.f,
			.MaxDepth = 1.f,
		};

		this->default_scissor_rect = D3D12_RECT{
			.left	= 0l,
			.top	= 0l,
			.right	= static_cast<int32>(platform::get_client_width(h_window)),
			.bottom = static_cast<int32>(platform::get_client_height(h_window)),
		};
	}
}	 // namespace age::graphics

namespace age::graphics
{
	render_surface_handle
	create_render_surface(platform::window_handle h_window) noexcept
	{
		auto id = g::render_surface_vec.emplace_back();
		g::render_surface_vec[id].init(h_window);

		return render_surface_handle{ .id = id };
	}

	render_surface_handle
	find_render_surface(platform::window_handle h_window) noexcept
	{
		for (auto nth : std::views::iota(0ul) | std::views::take(g::render_surface_vec.size()))
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

	monitor_data
	get_monitor_data(render_surface_handle h_render_surface) noexcept
	{
		auto res = monitor_data{};

		c_auto& rs = g::render_surface_vec[h_render_surface];

		res.min_luminance			 = rs.min_luminance;
		res.max_luminance			 = rs.max_luminance;
		res.max_full_frame_luminance = rs.max_full_frame_luminance;
		res.monitor_name			 = rs.monitor_name;
		return res;
	}

	e::color_space_kind
	get_color_space(render_surface_handle h_render_surface) noexcept
	{
		return g::render_surface_vec[h_render_surface].color_space;
	}

	void
	request_change_color_space(render_surface_handle h_render_surface, e::color_space_kind preference_color_space) noexcept
	{
		g::render_surface_vec[h_render_surface].preference_color_space = preference_color_space;
		request::create<request::type::change_color_space>(h_render_surface);
	}
}	 // namespace age::graphics

#endif