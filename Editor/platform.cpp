#include "pch.h"
#include "editor_common.h"
#include "platform.h"
#include "editor_style.h"
#include "stb_image.h"

// extern editor_context* GEctx;

const ImVec2& editor::platform::get_window_pos()
{
	return GImGui->CurrentWindow->Pos;
}

const ImVec2& editor::platform::get_window_size()
{
	return GImGui->CurrentWindow->Size;
}

const ImVec2& editor::platform::get_monitor_size()
{
	return ImGui::GetPlatformIO().Monitors[0].MainSize;
}

const ImVec2& editor::platform::get_mouse_pos()
{
	return GImGui->IO.MousePos;
}

float editor::platform::get_mouse_wheel()
{
	return GImGui->IO.MouseWheel;
}

void editor::platform::add_mouse_source_event(ImGuiMouseSource source)
{
	GImGui->IO.AddMouseSourceEvent(source);
}

void editor::platform::add_mouse_button_event(ImGuiMouseButton mouse_button, bool down)
{
	GImGui->IO.AddMouseButtonEvent(mouse_button, down);
}

bool editor::platform::is_key_down(ImGuiKey key)
{
	return ImGui::IsKeyDown(key);
}

bool editor::platform::is_key_pressed(ImGuiKey key)
{
	return ImGui::IsKeyPressed(key);
}

viewport_info editor::platform::get_main_viewport()
{
	return { GImGui->Viewports[0] };
}

window_info editor::platform::get_window_info()
{
	return { ImGui::GetCurrentWindowRead() };
}

void editor::platform::set_next_window_pos(const ImVec2& pos, ImGuiCond cond, const ImVec2& pivot)
{
	assert(cond == 0 || (cond != 0 && (cond & (cond - 1)) == 0));	 // Make sure the user doesn't attempt to combine multiple condition flags.
	GImGui->NextWindowData.Flags	   |= ImGuiNextWindowDataFlags_HasPos;
	GImGui->NextWindowData.PosVal		= pos;
	GImGui->NextWindowData.PosPivotVal	= pivot;
	GImGui->NextWindowData.PosCond		= cond ? cond : ImGuiCond_Always;
	GImGui->NextWindowData.PosUndock	= true;
}

void editor::platform::set_next_window_size(const ImVec2& size, ImGuiCond cond)
{
	assert(cond == 0 || (cond != 0 && (cond & (cond - 1)) == 0));	 // Make sure the user doesn't attempt to combine multiple condition flags.
	GImGui->NextWindowData.Flags	|= ImGuiNextWindowDataFlags_HasSize;
	GImGui->NextWindowData.SizeVal	 = size;
	GImGui->NextWindowData.SizeCond	 = cond ? cond : ImGuiCond_Always;
}

void editor::platform::set_next_window_viewport(ImGuiID id)
{
	GImGui->NextWindowData.Flags	  |= ImGuiNextWindowDataFlags_HasViewport;
	GImGui->NextWindowData.ViewportId  = id;
}

bool editor::platform::mouse_clicked(ImGuiMouseButton mouse_button)
{
	return GImGui->IO.MouseClicked[mouse_button];
}

bool editor::platform::is_window_maximized(void* hwnd)
{
	WINDOWPLACEMENT placement = { 0 };
	placement.length		  = sizeof(WINDOWPLACEMENT);
	if (::GetWindowPlacement((HWND)hwnd, &placement))
	{
		return placement.showCmd == SW_SHOWMAXIMIZED;
	}

	return false;
}

bool editor::platform::is_in_client(const void* p_screen_pos)
{
	auto& screen_pos = *(POINT*)p_screen_pos;
	return screen_pos.y >= CAPTION_HIGHT * GEctx->dpi_scale or GEctx->main_menu_rect.Contains(ImVec2((float)screen_pos.x, (float)screen_pos.y));
}

//=================================================================================================================================================================================================

namespace
{
	struct FrameContext
	{
		ID3D12CommandAllocator* CommandAllocator;
		UINT64					FenceValue;
	};

	static const int	   NUM_FRAMES_IN_FLIGHT					= 3;
	static FrameContext	   g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
	static UINT			   g_frameIndex							= 0;
	static ID3D12Resource* g_p_icon_texture						= NULL;
	static WNDCLASSEXW	   wc;
	static HWND			   hwnd;

	static const int				   NUM_BACK_BUFFERS								  = 3;
	static ID3D12Device*			   g_pd3dDevice									  = nullptr;
	static ID3D12DescriptorHeap*	   g_pd3dRtvDescHeap							  = nullptr;
	static ID3D12DescriptorHeap*	   g_pd3dSrvDescHeap							  = nullptr;
	static ID3D12CommandQueue*		   g_pd3dCommandQueue							  = nullptr;
	static ID3D12GraphicsCommandList*  g_pd3dCommandList							  = nullptr;
	static ID3D12Fence*				   g_fence										  = nullptr;
	static HANDLE					   g_fenceEvent									  = nullptr;
	static UINT64					   g_fenceLastSignaledValue						  = 0;
	static IDXGISwapChain3*			   g_pSwapChain									  = nullptr;
	static HANDLE					   g_hSwapChainWaitableObject					  = nullptr;
	static ID3D12Resource*			   g_mainRenderTargetResource[NUM_BACK_BUFFERS]	  = {};
	static D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
}	 // namespace

namespace
{
	void WaitForLastSubmittedFrame()
	{
		FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue == 0)
			return;	   // No fence was signaled

		frameCtx->FenceValue = 0;
		if (g_fence->GetCompletedValue() >= fenceValue)
			return;

		g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}

	FrameContext* WaitForNextFrameResources()
	{
		UINT nextFrameIndex = g_frameIndex + 1;
		g_frameIndex		= nextFrameIndex;

		HANDLE waitableObjects[]  = { g_hSwapChainWaitableObject, nullptr };
		DWORD  numWaitableObjects = 1;

		FrameContext* frameCtx	 = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
		UINT64		  fenceValue = frameCtx->FenceValue;
		if (fenceValue != 0)	// means no fence was signaled
		{
			frameCtx->FenceValue = 0;
			g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
			waitableObjects[1] = g_fenceEvent;
			numWaitableObjects = 2;
		}

		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		return frameCtx;
	}

	void CreateRenderTarget()
	{
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		{
			ID3D12Resource* pBackBuffer = nullptr;
			g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
			g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
			g_mainRenderTargetResource[i] = pBackBuffer;
		}
	}

	// Simple helper function to load an image into a DX12 texture with common settings
	// Returns true on success, with the SRV CPU handle having an SRV for the newly-created texture placed in it (srv_cpu_handle must be a handle in a valid descriptor heap)
	bool LoadTextureFromFile(const char* filename, ID3D12Device* d3d_device, D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle, ID3D12Resource** out_tex_resource, int* out_width, int* out_height)
	{
		// Load from disk into a raw RGBA buffer
		int			   image_width	= 0;
		int			   image_height = 0;
		unsigned char* image_data	= stbi_load(filename, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
			return false;

		// Create texture resource
		D3D12_HEAP_PROPERTIES props;
		memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));
		props.Type				   = D3D12_HEAP_TYPE_DEFAULT;
		props.CPUPageProperty	   = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		D3D12_RESOURCE_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment			= 0;
		desc.Width				= image_width;
		desc.Height				= image_height;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource* pTexture = NULL;
		d3d_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
											D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&pTexture));

		// Create a temporary upload resource to move the data in
		UINT uploadPitch		= (image_width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		UINT uploadSize			= image_height * uploadPitch;
		desc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment			= 0;
		desc.Width				= uploadSize;
		desc.Height				= 1;
		desc.DepthOrArraySize	= 1;
		desc.MipLevels			= 1;
		desc.Format				= DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count	= 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags				= D3D12_RESOURCE_FLAG_NONE;

		props.Type				   = D3D12_HEAP_TYPE_UPLOAD;
		props.CPUPageProperty	   = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		ID3D12Resource* uploadBuffer = NULL;
		HRESULT			hr			 = d3d_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
																		   D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadBuffer));
		IM_ASSERT(SUCCEEDED(hr));

		// Write pixels into the upload resource
		void*		mapped = NULL;
		D3D12_RANGE range  = { 0, uploadSize };
		hr				   = uploadBuffer->Map(0, &range, &mapped);
		IM_ASSERT(SUCCEEDED(hr));
		for (int y = 0; y < image_height; y++)
			memcpy((void*)((uintptr_t)mapped + y * uploadPitch), image_data + y * image_width * 4, image_width * 4);
		uploadBuffer->Unmap(0, &range);

		// Copy the upload resource content into the real resource
		D3D12_TEXTURE_COPY_LOCATION srcLocation		   = {};
		srcLocation.pResource						   = uploadBuffer;
		srcLocation.Type							   = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
		srcLocation.PlacedFootprint.Footprint.Width	   = image_width;
		srcLocation.PlacedFootprint.Footprint.Height   = image_height;
		srcLocation.PlacedFootprint.Footprint.Depth	   = 1;
		srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource					= pTexture;
		dstLocation.Type						= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex			= 0;

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource   = pTexture;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		// Create a temporary command queue to do the copy with
		ID3D12Fence* fence = NULL;
		hr				   = d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		IM_ASSERT(SUCCEEDED(hr));

		HANDLE event = CreateEvent(0, 0, 0, 0);
		IM_ASSERT(event != NULL);

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type					   = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags					   = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask				   = 1;

		ID3D12CommandQueue* cmdQueue = NULL;
		hr							 = d3d_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue));
		IM_ASSERT(SUCCEEDED(hr));

		ID3D12CommandAllocator* cmdAlloc = NULL;
		hr								 = d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
		IM_ASSERT(SUCCEEDED(hr));

		ID3D12GraphicsCommandList* cmdList = NULL;
		hr								   = d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc, NULL, IID_PPV_ARGS(&cmdList));
		IM_ASSERT(SUCCEEDED(hr));

		cmdList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
		cmdList->ResourceBarrier(1, &barrier);

		hr = cmdList->Close();
		IM_ASSERT(SUCCEEDED(hr));

		// Execute the copy
		cmdQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&cmdList);
		hr = cmdQueue->Signal(fence, 1);
		IM_ASSERT(SUCCEEDED(hr));

		// Wait for everything to complete
		fence->SetEventOnCompletion(1, event);
		WaitForSingleObject(event, INFINITE);

		// Tear down our temporary command queue and release the upload resource
		cmdList->Release();
		cmdAlloc->Release();
		cmdQueue->Release();
		CloseHandle(event);
		fence->Release();
		uploadBuffer->Release();

		// Create a shader resource view for the texture
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format					  = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension			  = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels		  = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Shader4ComponentMapping	  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		d3d_device->CreateShaderResourceView(pTexture, &srvDesc, srv_cpu_handle);

		// Return results
		*out_tex_resource = pTexture;
		*out_width		  = image_width;
		*out_height		  = image_height;
		stbi_image_free(image_data);

		return true;
	}

	void CleanupRenderTarget()
	{
		WaitForLastSubmittedFrame();

		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			if (g_mainRenderTargetResource[i])
			{
				g_mainRenderTargetResource[i]->Release();
				g_mainRenderTargetResource[i] = nullptr;
			}
	}
}	 // namespace

bool editor::platform::CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC1 sd;
	{
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount		  = NUM_BACK_BUFFERS;
		sd.Width			  = 0;
		sd.Height			  = 0;
		sd.Format			  = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Flags			  = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		sd.BufferUsage		  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SampleDesc.Count	  = 1;
		sd.SampleDesc.Quality = 0;
		sd.SwapEffect		  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.AlphaMode		  = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.Scaling			  = DXGI_SCALING_STRETCH;
		sd.Stereo			  = FALSE;
	}

	// [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
	ID3D12Debug* pdx12Debug = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
		pdx12Debug->EnableDebugLayer();
#endif

	// Create device
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
		return false;

	// [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
	if (pdx12Debug != nullptr)
	{
		ID3D12InfoQueue* pInfoQueue = nullptr;
		g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		pInfoQueue->Release();
		pdx12Debug->Release();
	}
#endif

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.NumDescriptors				= NUM_BACK_BUFFERS;
		desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask					= 1;
		if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
			return false;

		SIZE_T						rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle		  = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
		{
			g_mainRenderTargetDescriptor[i]	 = rtvHandle;
			rtvHandle.ptr					+= rtvDescriptorSize;
		}
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type						= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors				= 2;	// for loading icon image
		desc.Flags						= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
			return false;
	}

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type					  = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask				  = 1;
		if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
			return false;
	}

	for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
		if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
			return false;

	if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK || g_pd3dCommandList->Close() != S_OK)
		return false;

	if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
		return false;

	g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (g_fenceEvent == nullptr)
		return false;

	{
		IDXGIFactory4*	 dxgiFactory = nullptr;
		IDXGISwapChain1* swapChain1	 = nullptr;
		if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
			return false;
		if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
			return false;
		if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
			return false;
		swapChain1->Release();
		dxgiFactory->Release();
		g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
		g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
	}

	CreateRenderTarget();
	return true;
}

unsigned long long editor::platform::load_icon_image(const char* path)
{
	// We need to pass a D3D12_CPU_DESCRIPTOR_HANDLE in ImTextureID, so make sure it will fit
	static_assert(sizeof(ImTextureID) >= sizeof(D3D12_CPU_DESCRIPTOR_HANDLE), "D3D12_CPU_DESCRIPTOR_HANDLE is too large to fit in an ImTextureID");

	int my_image_width	= 0;
	int my_image_height = 0;

	// Get CPU/GPU handles for the shader resource view
	// Normally your engine will have some sort of allocator for these - here we assume that there's an SRV descriptor heap in
	// g_pd3dSrvDescHeap with at least two descriptors allocated, and descriptor 1 is unused
	UINT						handle_increment		   = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	int							descriptor_index		   = 1;	   // The descriptor table index to use (not normally a hard-coded constant, but in this case we'll assume we have slot 1 reserved for us)
	D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle  = g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
	my_texture_srv_cpu_handle.ptr						  += (handle_increment * descriptor_index);
	D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle  = g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
	my_texture_srv_gpu_handle.ptr						  += (handle_increment * descriptor_index);

	// Load the texture from a file
	bool ret = LoadTextureFromFile(path, g_pd3dDevice, my_texture_srv_cpu_handle, &g_p_icon_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);

	return my_texture_srv_gpu_handle.ptr;
}

void editor::platform::new_frame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	COLORREF BORDER_COLOR = ::GetFocus() != NULL ? RGB(113, 96, 232) : RGB(61, 61, 61);
	::DwmSetWindowAttribute(
		(HWND)GEctx->hwnd, DWMWINDOWATTRIBUTE::DWMWA_BORDER_COLOR,
		&BORDER_COLOR, sizeof(BORDER_COLOR));
}

void editor::platform::render()
{
	ImGui::Render();
	ImVec4		  clear_color	= ImVec4(0.45f, 0.55f, 0.60f, 1.f);
	FrameContext* frameCtx		= WaitForNextFrameResources();
	UINT		  backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
	frameCtx->CommandAllocator->Reset();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags				   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = g_mainRenderTargetResource[backBufferIdx];
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
	g_pd3dCommandList->ResourceBarrier(1, &barrier);

	// Render Dear ImGui graphics
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
	g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
	g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	g_pd3dCommandList->ResourceBarrier(1, &barrier);
	g_pd3dCommandList->Close();

	g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

	// Update and Render additional Platform Windows
	if (GImGui->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)g_pd3dCommandList);
	}

	g_pSwapChain->Present(1, 0);	// Present with vsync
	// g_pSwapChain->Present(0, 0); // Present without vsync

	UINT64 fenceValue = g_fenceLastSignaledValue + 1;
	g_pd3dCommandQueue->Signal(g_fence, fenceValue);
	g_fenceLastSignaledValue = fenceValue;
	frameCtx->FenceValue	 = fenceValue;
}

void editor::platform::wm_size(LPARAM lParam, WPARAM wParam)
{
	if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
	{
		WaitForLastSubmittedFrame();
		CleanupRenderTarget();
		HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
		assert(SUCCEEDED(result) && "Failed to resize swapchain.");
		CreateRenderTarget();
	}
}

void editor::platform::CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain)
	{
		g_pSwapChain->SetFullscreenState(false, nullptr);
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}
	if (g_hSwapChainWaitableObject != nullptr)
	{
		CloseHandle(g_hSwapChainWaitableObject);
	}
	for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
		if (g_frameContext[i].CommandAllocator)
		{
			g_frameContext[i].CommandAllocator->Release();
			g_frameContext[i].CommandAllocator = nullptr;
		}
	if (g_pd3dCommandQueue)
	{
		g_pd3dCommandQueue->Release();
		g_pd3dCommandQueue = nullptr;
	}
	if (g_pd3dCommandList)
	{
		g_pd3dCommandList->Release();
		g_pd3dCommandList = nullptr;
	}
	if (g_pd3dRtvDescHeap)
	{
		g_pd3dRtvDescHeap->Release();
		g_pd3dRtvDescHeap = nullptr;
	}
	if (g_pd3dSrvDescHeap)
	{
		g_pd3dSrvDescHeap->Release();
		g_pd3dSrvDescHeap = nullptr;
	}
	if (g_fence)
	{
		g_fence->Release();
		g_fence = nullptr;
	}
	if (g_fenceEvent)
	{
		CloseHandle(g_fenceEvent);
		g_fenceEvent = nullptr;
	}
	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}

#ifdef DX12_ENABLE_DEBUG_LAYER
	IDXGIDebug1* pDebug = nullptr;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		pDebug->Release();
	}
#endif
}

HWND editor::platform::init(LRESULT(WndProc)(HWND, UINT, WPARAM, LPARAM))
{
	wc = WNDCLASSEXW { sizeof(WNDCLASSEXW), /*CS_CLASSDC*/ CS_HREDRAW | CS_VREDRAW | CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"AssembleGE Editor", nullptr };
	::RegisterClassExW(&wc);

	auto window_style = WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE;
	ImGui_ImplWin32_EnableDpiAwareness();
	BOOL USE_DARK_MODE = true;

	float dpi_scale		= ::GetDpiForSystem() / 96.f;
	auto  screen_size_x = ::GetSystemMetrics(SM_CXSCREEN);
	auto  screen_size_y = ::GetSystemMetrics(SM_CYSCREEN);
	hwnd				= ::CreateWindowW(wc.lpszClassName, L"AssembleGE Editor", window_style /*WS_DLGFRAME*/, screen_size_x / 10, screen_size_y / 10, screen_size_x * 4 / 5, screen_size_y * 4 / 5, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (editor::platform::CreateDeviceD3D(hwnd) is_false)
	{
		editor::platform::CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		assert(false);
		// return 1;
		return HWND {};
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags					|= ImGuiConfigFlags_NavEnableKeyboard;	  // Enable Keyboard Controls
	io.ConfigFlags					|= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls
	io.ConfigFlags					|= ImGuiConfigFlags_DockingEnable;		  // Enable Docking
	io.ConfigFlags					|= ImGuiConfigFlags_ViewportsEnable;	  // Enable Multi-Viewport / Platform Windows
	io.ConfigViewportsNoAutoMerge	 = true;
	io.ConfigViewportsNoTaskBarIcon	 = true;

	::ImGui_ImplWin32_Init(hwnd);
	::ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
						  DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
						  g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
						  g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	return hwnd;
}

void editor::platform::close()
{
	WaitForLastSubmittedFrame();

	g_p_icon_texture->Release();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

namespace
{
	std::vector<std::function<void()>> _on_wm_activated_vec;
}

void editor::platform::add_on_wm_activate(std::function<void()> func)
{
	_on_wm_activated_vec.emplace_back(func);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifndef GET_X_PARAM
	#define GET_X_PARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_PARAM
	#define GET_Y_PARAM(lp) ((int)(short)HIWORD(lp))
#endif

LRESULT WINAPI editor::platform::wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// todo change imgui source file
	if (msg == WM_NCMOUSEMOVE)
	{
		switch (wParam)
		{
		case HTCAPTION:
		{
			GEctx->caption_hovered_btn = Caption_Button_None;
			break;
		}
		case HTMINBUTTON:
		{
			GEctx->caption_hovered_btn = caption_button::Caption_Button_Min;
			break;
		}
		case HTMAXBUTTON:
		{
			GEctx->caption_hovered_btn = caption_button::Caption_Button_Max;
			break;
		}
		case HTCLOSE:
		{
			GEctx->caption_hovered_btn = caption_button::Caption_Button_Close;
			break;
		}
		}
	}

	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_SIZE:
	{
		editor::platform::wm_size(lParam, wParam);
		return 0;
	}
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			break;
		}

		std::ranges::for_each(_on_wm_activated_vec, [](auto func) { func(); });

		break;
	}
	case WM_SYSCOMMAND:
	{
		if ((wParam & 0xfff0) == SC_KEYMENU)	// Disable ALT application menu
			return 0;
		break;
	}
	case WM_CREATE:
	{
		RECT rcClient;
		GetWindowRect(hWnd, &rcClient);

		// Inform the application of the frame change.
		SetWindowPos(hWnd,
					 NULL,
					 rcClient.left, rcClient.top,
					 rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
					 SWP_FRAMECHANGED);
		return 0;
	}
	case WM_NCHITTEST:
	{
		// Let the default procedure handle resizing areas
		LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
		switch (hit)
		{
		case HTNOWHERE:
		case HTRIGHT:
		case HTLEFT:
		case HTTOPLEFT:
		case HTTOP:
		case HTTOPRIGHT:
		case HTBOTTOMRIGHT:
		case HTBOTTOM:
		case HTBOTTOMLEFT:
		{
			return hit;
		}
		}

		// Looks like adjustment happening in NCCALCSIZE is messing with the detection
		// of the top hit area so manually fixing that.
		UINT  dpi		   = GetDpiForWindow(hWnd);
		int	  frame_y	   = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int	  padding	   = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
		POINT cursor_point = { GET_X_PARAM(lParam), GET_Y_PARAM(lParam) };
		ScreenToClient(hWnd, &cursor_point);

		if (cursor_point.y > 0 && cursor_point.y < frame_y + padding)
		{
			return HTTOP;
		}

		if (editor::platform::is_in_client(&cursor_point))
		{
			GEctx->caption_hovered_btn = Caption_Button_None;
			GEctx->caption_held_btn	   = Caption_Button_None;
			return HTCLIENT;
		}

		float cxDiff = ImGui::GetMainViewport()->Size.x - cursor_point.x;
		if (cxDiff > CAPTION_BUTTON_SIZE.x * GEctx->dpi_scale * 3)
		{
			return HTCAPTION;
		}
		if (cxDiff > CAPTION_BUTTON_SIZE.x * GEctx->dpi_scale * 2)
		{
			return HTMINBUTTON;
		}
		if (cxDiff > CAPTION_BUTTON_SIZE.x * GEctx->dpi_scale)
		{
			return HTMAXBUTTON;
		}

		return HTCLOSE;
	}
	case WM_NCLBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	{
		POINT cursor_point = { GET_X_PARAM(lParam), GET_Y_PARAM(lParam) };
		ScreenToClient(hWnd, &cursor_point);

		if (GEctx->caption_hovered_btn == Caption_Button_None)
		{
			break;
			// return 0;
		}

		GEctx->caption_held_btn = GEctx->caption_hovered_btn;
		return 0;
	}
	case WM_NCLBUTTONUP:
	{
		if (GEctx->caption_held_btn != GEctx->caption_hovered_btn)
		{
			break;
		}

		GEctx->caption_held_btn = Caption_Button_None;

		switch (GEctx->caption_hovered_btn)
		{
		case Caption_Button_Min:
			::ShowWindow(hWnd, SW_MINIMIZE);
			return 0;
		case Caption_Button_Max:
		{
			int mode = editor::platform::is_window_maximized(GEctx->hwnd) ? SW_NORMAL : SW_MAXIMIZE;
			::ShowWindow(hWnd, mode);
		}
			return 0;
		case Caption_Button_Close:
			::PostMessageW(hWnd, WM_CLOSE, 0, 0);
			return 0;
		}

		break;
	}
	case WM_NCCALCSIZE:
	{
		if (!wParam)
		{
			break;
		}

		auto dpi = GetDpiForWindow(hWnd);

		auto frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
		auto frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		auto padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

		auto* params				= (NCCALCSIZE_PARAMS*)lParam;
		auto* requested_client_rect = params->rgrc;

		requested_client_rect->right  -= frame_x + padding;
		requested_client_rect->left	  += frame_x + padding;
		requested_client_rect->bottom -= frame_y + padding;

		if (editor::platform::is_window_maximized(hWnd))
		{
			requested_client_rect->top += padding;
		}

		return 0;
	}
	case WM_SETCURSOR:
	{
		// Show an arrow instead of the busy cursor
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
	}
	case WM_DPICHANGED:
	{
		// todo
		// only work when release mode? wtf?
		auto* rect = (RECT*)lParam;
		IM_ASSERT(LOWORD(wParam) == HIWORD(wParam));
		::SetWindowPos(hWnd, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER);
		editor::style::update_dpi_scale((float)LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI);
		return 0;
	}
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void editor::platform::window_loop(void (*editor_loop)())
{
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				done = true;
			}
		}
		if (done)
		{
			break;
		}

		// Start the Dear ImGui frame
		platform::new_frame();

		editor_loop();

		platform::render();
	}

	platform::close();
	::DestroyWindow((HWND)GEctx->hwnd);
}
