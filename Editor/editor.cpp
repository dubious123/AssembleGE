#include "pch.h"

#include "stb_image.h"
#include "editor.h"
#include "editor_view.h"
#include "game_project\game.h"

// #ifdef _DEBUG
//	#define DX12_ENABLE_DEBUG_LAYER
// #endif
//
// #ifdef DX12_ENABLE_DEBUG_LAYER
//	#include <dxgidebug.h>
//	#pragma comment(lib, "dxguid.lib")
// #endif

// These are defined in <windowsx.h> but we don't want to pull in whole header
// And we need them because coordinates are signed when you have multi-monitor setup.
#ifndef GET_X_PARAM
	#define GET_X_PARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_PARAM
	#define GET_Y_PARAM(lp) ((int)(short)HIWORD(lp))
#endif

editor_context* GEctx = nullptr;

struct FrameContext
{
	ID3D12CommandAllocator* CommandAllocator;
	UINT64					FenceValue;
};

// Data
static const int	   NUM_FRAMES_IN_FLIGHT					= 3;
static FrameContext	   g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT			   g_frameIndex							= 0;
static ID3D12Resource* g_p_icon_texture						= NULL;
static WNDCLASSEXW	   wc;

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

// Forward declarations of helper functions
namespace editor
{
	namespace
	{
		void _custom_buttons();
		void _custom_caption();
		void _load_ctx_menu();
	}	 // namespace
}	 // namespace editor

bool		  CreateDeviceD3D(HWND hWnd);
void		  CleanupDeviceD3D();
void		  CreateRenderTarget();
void		  CleanupRenderTarget();
void		  WaitForLastSubmittedFrame();
bool		  LoadTextureFromFile(const char* filename, ID3D12Device* d3d_device, D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle, ID3D12Resource** out_tex_resource, int* out_width, int* out_height);
FrameContext* WaitForNextFrameResources();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	void _reset_colors()
	{
		ImGuiStyle& style							 = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text]					 = COL_TEXT;
		style.Colors[ImGuiCol_TextDisabled]			 = COL_TEXT_DISABLED;
		style.Colors[ImGuiCol_WindowBg]				 = COL_BLACK;		   // COL_BG_WINDOW;
		style.Colors[ImGuiCol_ChildBg]				 = COL_BLACK;		   // COL_BG_WINDOW;	   // COL_BG_POPUP;
		style.Colors[ImGuiCol_PopupBg]				 = COL_BG_ACTIVE;	   // COL_BG_WINDOW;	   // COL_BG_POPUP;
		style.Colors[ImGuiCol_Border]				 = COL_BD_SELECTED;	   // ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		style.Colors[ImGuiCol_BorderShadow]			 = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg]				 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered]		 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive]		 = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
		style.Colors[ImGuiCol_TitleBg]				 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TitleBgActive]		 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed]		 = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		style.Colors[ImGuiCol_MenuBarBg]			 = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg]			 = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab]		 = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered]	 = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]	 = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		style.Colors[ImGuiCol_CheckMark]			 = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab]			 = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive]		 = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
		style.Colors[ImGuiCol_Button]				 = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_ButtonHovered]		 = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]			 = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		style.Colors[ImGuiCol_Header]				 = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		style.Colors[ImGuiCol_HeaderHovered]		 = COL_BG_SELECTED;	   // ImVec4(0.25f, 0.25f, 0.25f, 1.00f); //selectable hovered
		style.Colors[ImGuiCol_HeaderActive]			 = COL_GRAY_2;		   // ImVec4(0.67f, 0.67f, 0.67f, 0.39f); //selectable active
		style.Colors[ImGuiCol_Separator]			 = style.Colors[ImGuiCol_Border];
		style.Colors[ImGuiCol_SeparatorHovered]		 = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
		style.Colors[ImGuiCol_SeparatorActive]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_ResizeGrip]			 = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_ResizeGripHovered]	 = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_Tab]					 = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
		style.Colors[ImGuiCol_TabHovered]			 = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
		style.Colors[ImGuiCol_TabActive]			 = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocused]			 = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_TabUnfocusedActive]	 = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_DockingPreview]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		style.Colors[ImGuiCol_DockingEmptyBg]		 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		style.Colors[ImGuiCol_PlotLines]			 = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered]		 = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram]		 = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered]	 = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg]		 = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		style.Colors[ImGuiCol_DragDropTarget]		 = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
		style.Colors[ImGuiCol_NavHighlight]			 = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		style.Colors[ImGuiCol_NavWindowingDimBg]	 = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_ModalWindowDimBg]		 = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		if (GImGui->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
	}

	void _reset_style()
	{
		ImGuiStyle& style		 = ImGui::GetStyle();
		style.WindowBorderSize	 = 1.0f;
		style.ChildBorderSize	 = 1.0f;
		style.PopupBorderSize	 = 1.0f;
		style.FrameBorderSize	 = 1.0f;
		style.TabBorderSize		 = 0.0f;
		style.WindowRounding	 = 0.0f;
		style.ChildRounding		 = 0.0f;
		style.PopupRounding		 = 0.0f;
		style.FrameRounding		 = 2.3f;
		style.ScrollbarRounding	 = 0.0f;
		style.GrabRounding		 = 2.3f;
		style.TabRounding		 = 0.0f;
		style.FramePadding		 = ImVec2(6.f, 6.f);	// ImVec2(6.f, 6.f);
		style.PopupBorderSize	 = .4f;
		style.ItemSpacing.y		 = 3.f;
		style.ItemInnerSpacing.x = 5.f;
		// style.ItemSpacing		= ImVec2(8.4f, 3.f);	// ImVec2(8.4f, 3.f);					   // 3.f;
		style.WindowPadding = ImVec2(8.f, 8.f);	   // 3.f;
		style.WindowMinSize = ImVec2(1.f, 1.f);

		// style.SelectableTextAlign = ImVec2(0, 0.5f);
		// style.ItemSpacing		= MENU_ITEM_PADDING * 2;				// ImVec2(8.4f, 3.f);					   // 3.f;
		// style.WindowPadding		= POPUP_PADDING + style.ItemSpacing;	// 3.f;
		//     When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		if (GImGui->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
		}
	}

	void _update_dpi_scale(float new_dpi_scale)
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui_ImplDX12_InvalidateDeviceObjects();

		// Setup Dear ImGui style
		ImGuiStyle& style	 = ImGui::GetStyle();
		ImGuiStyle	styleold = style;
		style				 = ImGuiStyle();
		_reset_style();
		CopyMemory(style.Colors, styleold.Colors, sizeof(style.Colors));	// Restore colors

		style.ScaleAllSizes(new_dpi_scale);
		GEctx->dpi_scale = new_dpi_scale;
		io.Fonts->Clear();

		auto res = std::filesystem::absolute("Resources/arial.ttf");

		GEctx->p_font_arial_default_13_5 = io.Fonts->AddFontFromFileTTF("Resources/arial.ttf", 13.5f * new_dpi_scale);
		GEctx->p_font_arial_bold_13_5	 = io.Fonts->AddFontFromFileTTF("Resources/arialbd.ttf", 13.5f * new_dpi_scale);

		GEctx->p_font_arial_default_16 = io.Fonts->AddFontFromFileTTF("Resources/arial.ttf", 16.f * new_dpi_scale);
		GEctx->p_font_arial_bold_16	   = io.Fonts->AddFontFromFileTTF("Resources/arialbd.ttf", 16.f * new_dpi_scale);

		GEctx->p_font_arial_default_18 = io.Fonts->AddFontFromFileTTF("Resources/arial.ttf", 18.f * new_dpi_scale);
		GEctx->p_font_arial_bold_18	   = io.Fonts->AddFontFromFileTTF("Resources/arialbd.ttf", 18.f * new_dpi_scale);

		GImGui->Font = GEctx->p_font_arial_default_13_5;

		ImGui_ImplDX12_CreateDeviceObjects();

		GEctx->dpi_changed = true;
		// todo
		// Editor::View::Update_Dpi_Scale();
	}

	bool _is_window_maximized(void* hwnd)
	{
		WINDOWPLACEMENT placement = { 0 };
		placement.length		  = sizeof(WINDOWPLACEMENT);
		if (::GetWindowPlacement((HWND)hwnd, &placement))
		{
			return placement.showCmd == SW_SHOWMAXIMIZED;
		}

		return false;
	}

	bool inline _is_mouse_in_client(POINT mouseScreenPos)
	{
		return mouseScreenPos.y >= CAPTION_HIGHT * editor::platform::dpi_scale() or GEctx->main_menu_rect.Contains(ImVec2((float)mouseScreenPos.x, (float)mouseScreenPos.y));
	}

	void _close()
	{
		WaitForLastSubmittedFrame();

		// Cleanup
		g_p_icon_texture->Release();

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		CleanupDeviceD3D();
		::DestroyWindow((HWND)GEctx->hwnd);
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);

		delete GEctx;
	}
}	 // namespace

void editor::init()
{
	// Create application window
	wc = { sizeof(wc), /*CS_CLASSDC*/ CS_HREDRAW | CS_VREDRAW | CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"AssembleGE Editor", nullptr };
	::RegisterClassExW(&wc);

	auto window_style = WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE;
	ImGui_ImplWin32_EnableDpiAwareness();
	BOOL USE_DARK_MODE = true;

	float dpi_scale		= ::GetDpiForSystem() / 96.f;
	auto  screen_size_x = ::GetSystemMetrics(SM_CXSCREEN);
	auto  screen_size_y = ::GetSystemMetrics(SM_CYSCREEN);
	HWND  hwnd			= ::CreateWindowW(wc.lpszClassName, L"AssembleGE Editor", window_style /*WS_DLGFRAME*/, screen_size_x / 10, screen_size_y / 10, screen_size_x * 4 / 5, screen_size_y * 4 / 5, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		// return 1;
		return;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	GEctx			 = new editor_context();
	GEctx->hwnd		 = hwnd;
	GEctx->dpi_scale = ::GetDpiForSystem() / 96.f;

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags					|= ImGuiConfigFlags_NavEnableKeyboard;	  // Enable Keyboard Controls
	io.ConfigFlags					|= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls
	io.ConfigFlags					|= ImGuiConfigFlags_DockingEnable;		  // Enable Docking
	io.ConfigFlags					|= ImGuiConfigFlags_ViewportsEnable;	  // Enable Multi-Viewport / Platform Windows
	io.ConfigViewportsNoAutoMerge	 = true;
	io.ConfigViewportsNoTaskBarIcon	 = true;

	_reset_colors();
	_update_dpi_scale(dpi_scale);

	// GEctx->P_Font_Arial_Default = io.Fonts->AddFontFromFileTTF("../Resources/arial.ttf", 13.5f * GEctx->Dpi_Scale);
	// GEctx->P_Font_Arial_Bold	= io.Fonts->AddFontFromFileTTF("../Resources/arialbd.ttf", 13.5f * GEctx->Dpi_Scale);
	// Reset_Colors();
	// Reset_Style();
	// ImGui::GetStyle().ScaleAllSizes(GEctx->Dpi_Scale);

	// io.ConfigViewportsNoAutoMerge = true;
	// io.FontGlobalScale = GEctx->Dpi_Scale;

	// GImGui->WindowsHoverPadding = ImVec2(GEctx->Dpi_Scale, GEctx->Dpi_Scale);

	// Setup Platform/Renderer backends
	::ImGui_ImplWin32_Init(hwnd);
	::ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
						  DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
						  g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
						  g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

#pragma region Load Icon Image
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
	bool ret = LoadTextureFromFile("Resources/AssembleGE_Icon.png", g_pd3dDevice, my_texture_srv_cpu_handle, &g_p_icon_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);

	GEctx->icon_texture_id = my_texture_srv_gpu_handle.ptr;
#pragma endregion

	// editor::id::init();
	//_load_ctx_menu();
	editor::logger::init();
	editor::view::init();
	editor::game::init();
	// Editor::UndoRedo::Init();
}

void editor::run()
{
	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.f);

	// DragAcceptFiles(hwnd, TRUE);

	// Main loop
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

		COLORREF BORDER_COLOR = ::GetFocus() != NULL ? RGB(113, 96, 232) : RGB(61, 61, 61);
		::DwmSetWindowAttribute(
			(HWND)GEctx->hwnd, DWMWINDOWATTRIBUTE::DWMWA_BORDER_COLOR,
			&BORDER_COLOR, sizeof(BORDER_COLOR));

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (GEctx->dpi_changed)
		{
			editor::view::update_dpi_scale();
			GEctx->dpi_changed = false;
		}

		_custom_caption();

		editor::view::show();

		ImGui::ShowDemoWindow();

		editor::on_frame_end();
		ImGui::Render();

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

	_close();
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
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

void CleanupDeviceD3D()
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
		{
			WaitForLastSubmittedFrame();
			CleanupRenderTarget();
			HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
			assert(SUCCEEDED(result) && "Failed to resize swapchain.");
			CreateRenderTarget();
		}

		return 0;
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

		if (_is_mouse_in_client(cursor_point))
		{
			GEctx->caption_hovered_btn = Caption_Button_None;
			GEctx->caption_held_btn	   = Caption_Button_None;
			return HTCLIENT;
		}

		float cxDiff = ImGui::GetMainViewport()->Size.x - cursor_point.x;
		if (cxDiff > CAPTION_BUTTON_SIZE.x * editor::platform::dpi_scale() * 3)
		{
			return HTCAPTION;
		}
		if (cxDiff > CAPTION_BUTTON_SIZE.x * editor::platform::dpi_scale() * 2)
		{
			return HTMINBUTTON;
		}
		if (cxDiff > CAPTION_BUTTON_SIZE.x * editor::platform::dpi_scale())
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
			int mode = _is_window_maximized(GEctx->hwnd) ? SW_NORMAL : SW_MAXIMIZE;
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

		if (_is_window_maximized(hWnd))
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
		_update_dpi_scale((float)LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI);
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

namespace editor
{
	namespace
	{
		static constinit auto _context_menu_xml_path = "Resources/Editor_CtxMenu.xml";

		auto _ctx_item_doc		= pugi::xml_document();
		auto _ctx_item_xml_node = pugi::xml_node();

		auto _cmd_id			   = 0;
		auto _show_ctx_menu		   = false;
		auto _selected_vec		   = std::vector<editor_id>();
		auto _shortcut_command_vec = std::vector<const editor_command*>();
		// auto _current_select_ctx_item = editor_id();

		auto _command_lut  = std::vector<const editor_command*>();
		auto _ctx_node_lut = std::vector<pugi::xml_node>(DataType_Count);

		void _load_ctx_menu()
		{
			auto index = 0, parent_index = -1;
			int	 ctx_item_count = 0, item_index = 0;
			_ctx_item_doc.load_file(_context_menu_xml_path);
			_ctx_item_xml_node = _ctx_item_doc.first_child().first_child();
			for (auto node = _ctx_item_xml_node.next_sibling(); node; node = node.next_sibling())
			{
				if (strcmp(node.attribute("name").value(), "Scene") == 0)
				{
					_ctx_node_lut[DataType_Scene] = node;
				}
				else if (strcmp(node.attribute("name").value(), "World") == 0)
				{
					_ctx_node_lut[DataType_World] = node;
				}
				else if (strcmp(node.attribute("name").value(), "Entity") == 0)
				{
					_ctx_node_lut[DataType_Entity] = node;
				}
				else if (strcmp(node.attribute("name").value(), "Component") == 0)
				{
					_ctx_node_lut[DataType_Component] = node;
				}
			}
		}

		void _context_item(const pugi::xml_node& node)
		{
			bool has_child = node.first_child();
			auto name	   = node.attribute("name").value();
			if (has_child)
			{
				if (editor::widgets::begin_menu(name, nullptr))
				{
					for (auto child = node.first_child(); child; child = child.next_sibling())
					{
						_context_item(child);
					}

					editor::widgets::end_menu();
				}
			}
			else
			{
				auto	  command_id = node.attribute("__cmd_id") ? node.attribute("__cmd_id").as_int() : -1;
				editor_id arg		 = node.attribute("__arg") ? node.attribute("__arg").as_ullong() : INVALID_ID;
				if (editor::widgets::menu_item(name,
											   nullptr,
											   node.attribute("shortcut").value(),
											   false,
											   command_id >= 0 ? _command_lut[command_id]->can_execute(arg) : false))
				{
					_command_lut[command_id]->execute(arg);
				}
			}

			if (node.attribute("separator"))
			{
				widgets::separator();
			}
		}

		void _menu_bar()
		{
			static auto window_size	 = ImVec2();
			auto		window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;

			widgets::set_cursor_pos({ CAPTION_ICON_CPOS.x * platform::dpi_scale(), CAPTION_ICON_CPOS.y * platform::dpi_scale() });
			widgets::image(GEctx->icon_texture_id, ImVec2(CAPTION_ICON_SIZE.x * platform::dpi_scale(), CAPTION_ICON_SIZE.y * platform::dpi_scale()));
			widgets::sameline();

			style::push_var(ImGuiStyleVar_FramePadding, ImVec2(8.4f, 3.f) * platform::dpi_scale());
			style::push_var(ImGuiStyleVar_ItemSpacing, ImVec2());
			style::push_color(ImGuiCol_HeaderHovered, COL_BG_SELECTED);
			style::push_color(ImGuiCol_HeaderActive, style::get_color_v4(ImGuiCol_PopupBg));
			platform::set_next_window_pos(platform::get_window_pos() /*GImGui->CurrentWindow->Pos*/ + ImVec2(widgets::get_cursor_pos_x(), (CAPTION_ICON_CPOS.y * 2 * platform::dpi_scale() + CAPTION_ICON_SIZE.y * platform::dpi_scale() - (style::font_size() + style::frame_padding().y * 2)) * 0.5f));
			if (widgets::begin_child("Main Menu Bar", window_size, false, window_flags))
			{
				const auto& main_menu_node						= _ctx_item_xml_node;	 //_ctx_items[0];
				platform::get_window_info().layout()			= ImGuiLayoutType_Horizontal;
				platform::get_window_info().menubar_appending() = true;
				platform::get_window_info().nav_layer()			= ImGuiNavLayer_Menu;
				window_size										= ImVec2();
				style::push_var(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f) * platform::dpi_scale());

				for (auto header_node = main_menu_node.first_child(); header_node; header_node = header_node.next_sibling())
				{
					_context_item(header_node);

					window_size.x += widgets::get_item_rect().GetSize().x;
					window_size.y  = widgets::get_item_rect().GetSize().y;
				}

				window_size.y			  += 1;
				GEctx->main_menu_rect.Max  = widgets::get_item_rect().Max;

				style::pop_var(1);
			}
			style::pop_var(2);
			style::pop_color(2);
			widgets::end_child();
		}

		void _custom_buttons()
		{
			auto  dpi_scale					  = platform::dpi_scale();
			auto& style						  = GImGui->Style;
			auto  caption_button_icon_width	  = 5.f * dpi_scale;
			auto  caption_restore_icon_offset = 2.f * dpi_scale;
			auto  caption_button_size		  = ImVec2(CAPTION_BUTTON_SIZE.x * dpi_scale, CAPTION_BUTTON_SIZE.y * dpi_scale);
			auto  draw_pos					  = ImVec2(platform::get_window_pos().x + platform::get_window_size().x - caption_button_size.x * 3, platform::get_window_pos().y);
			auto  caption_button_icon_center  = ImVec2(draw_pos.x + caption_button_size.x / 2, draw_pos.y + caption_button_size.y / 2);

			if (GEctx->caption_hovered_btn != Caption_Button_None)
			{
				auto p_min = ImVec2(draw_pos.x + CAPTION_BUTTON_SIZE.x * (int)GEctx->caption_hovered_btn * platform::dpi_scale(), draw_pos.y);
				auto p_max = ImVec2(p_min.x + CAPTION_BUTTON_SIZE.x * platform::dpi_scale(), draw_pos.y + CAPTION_BUTTON_SIZE.y * platform::dpi_scale());
				widgets::draw_rect_filled({ p_min, p_max }, style::get_color_u32(GEctx->caption_held_btn == GEctx->caption_hovered_btn ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered));
			}

			widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y), ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y), style::get_color_u32(ImGuiCol_Text), dpi_scale);

			draw_pos.x					 += caption_button_size.x;
			caption_button_icon_center.x += caption_button_size.x;

			if (_is_window_maximized(GEctx->hwnd))
			{
				widgets::draw_rect(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width + caption_restore_icon_offset),
								   ImVec2(caption_button_icon_center.x + caption_button_icon_width - caption_restore_icon_offset, caption_button_icon_center.y + caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), 0.f, ImDrawFlags_None, dpi_scale);

				widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width + caption_restore_icon_offset),
								   ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);

				widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width),
								   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);

				widgets::draw_line(ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width),
								   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset), style::get_color_u32(ImGuiCol_Text), dpi_scale);

				widgets::draw_line(ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset),
								   ImVec2(caption_button_icon_center.x + caption_button_icon_width - caption_restore_icon_offset, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset), style::get_color_u32(ImGuiCol_Text), dpi_scale);
			}
			else
			{
				widgets::draw_rect(ImVec2(draw_pos.x + caption_button_size.x / 2 - caption_button_icon_width, draw_pos.y + caption_button_size.y / 2 - caption_button_icon_width), ImVec2(draw_pos.x + caption_button_size.x / 2 + caption_button_icon_width, draw_pos.y + caption_button_size.y / 2 + caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), 0.f, ImDrawFlags_None, dpi_scale);
			}

			draw_pos.x					 += caption_button_size.x;
			caption_button_icon_center.x += caption_button_size.x;

			widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width),
							   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);
			widgets::draw_line(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width),
							   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width), style::get_color_u32(ImGuiCol_Text), dpi_scale);
		}

		void _custom_caption()
		{
			auto viewport	  = platform::get_main_viewport();
			auto window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
			auto menuSize	  = ImVec2(viewport.size().x, CAPTION_HIGHT * platform::dpi_scale());

			style::push_color(ImGuiCol_WindowBg, style::get_color_v4(ImGuiCol_TitleBg));
			style::push_color(ImGuiCol_ChildBg, style::get_color_v4(ImGuiCol_TitleBg));
			style::push_var(ImGuiStyleVar_WindowBorderSize, 0.0f);
			style::push_var(ImGuiStyleVar_WindowPadding, ImVec2());

			platform::set_next_window_viewport(viewport.id());
			platform::set_next_window_pos(viewport.pos());
			platform::set_next_window_size(menuSize);
			if (widgets::begin("Caption", nullptr, window_flags))
			{
				_menu_bar();
				_custom_buttons();
			}

			style::pop_var(2);
			style::pop_color(2);

			widgets::end();
		}
	}	 // namespace

	void on_frame_end()
	{
		static bool close_ctx_popup = false;

		style::push_var(ImGuiStyleVar_FramePadding, ImVec2(8.4f, 3.f) * platform::dpi_scale());
		style::push_color(ImGuiCol_Header, style::get_color_v4(ImGuiCol_HeaderHovered));
		if (widgets::begin_popup("ctx menu"))
		{
			if (is_selection_vec_empty() is_false)
			{
				for (auto node = _ctx_node_lut[get_current_selection().type()].first_child(); node; node = node.next_sibling())
				{
					_context_item(node);
				}
			}
			else
			{
				close_ctx_popup = true;
			}

			if (close_ctx_popup)
			{
				widgets::close_popup();
				close_ctx_popup = false;
			}

			widgets::end_popup();
		}
		style::pop_color();
		style::pop_var();

		for (auto p_command : _shortcut_command_vec)
		{
			constexpr auto key_mask = 0x07ff;

			auto mode_shift = (p_command->_shortcut_key & ImGuiMod_Shift) != 0;
			auto mode_ctrl	= (p_command->_shortcut_key & ImGuiMod_Ctrl) != 0;
			auto key		= p_command->_shortcut_key & key_mask;
			auto res		= true;

			//               has mod / does not have mod
			// pressed         0             X
			// not pressed     X             O            not A xor B
			res &= (mode_shift == platform::is_key_down(ImGuiMod_Shift));
			res &= (mode_ctrl == platform::is_key_down(ImGuiMod_Ctrl));
			res &= ((key != ImGuiKey_None) == platform::is_key_pressed((ImGuiKey)key));

			if (res)
			{
				(*p_command)(/*current_source()*/);
				close_ctx_popup = true;
			}
		}

		if (_show_ctx_menu)
		{
			widgets::open_popup("ctx menu");
			_show_ctx_menu = false;
		}

		widgets::on_frame_end();
	}

	bool add_context_item(std::string path, const editor_command* p_command, editor_id id)
	{
		auto item_node = _ctx_item_xml_node.parent();
		auto stream	   = std::istringstream(path);
		for (std::string str; std::getline(stream, str, '\\');)
		{
			auto node = item_node.find_child([str](pugi::xml_node item) { return strcmp(item.attribute("name").value(), str.c_str()) == 0; });
			if (node.empty())
			{
				auto child = item_node.append_child("ctx_item");
				child.append_attribute("name").set_value(str.c_str());
				item_node = child;
			}
			else
			{
				item_node = node;
			}
		}

		// todo
		if (p_command is_nullptr) return false;

		_command_lut.emplace_back(p_command);

		item_node.append_attribute("__cmd_id").set_value(_cmd_id++);
		if (id.value != INVALID_ID)
		{
			item_node.append_attribute("__arg").set_value(id.value);
		}

		assert(_cmd_id == _command_lut.size());


		if (p_command->_shortcut_key != ImGuiKey_None)
		{
			_shortcut_command_vec.push_back(p_command);
		}

		return true;
	}

	void on_project_loaded()
	{
		_load_ctx_menu();

		logger::clear();
		undoredo::on_project_loaded();
		models::on_project_loaded();
		view::on_project_loaded();
		game::on_project_loaded();
		editor::game::save_project_open_datas();
		_selected_vec.clear();
	}

	void on_project_unloaded()
	{
		models::on_project_unloaded();
		//_cmd_id = 0;
		_selected_vec.clear();
		id::reset();
		//_current_select_ctx_item = INVALID_ID;
	}
}	 // namespace editor

namespace editor::id
{
	namespace
	{
		std::array<editor_id, DataType_Count> _next_id_vec = []() {
			std::array<editor_id, DataType_Count> arr;
			for (auto i = 0; i < DataType_Count; ++i)
			{
				arr[i] = editor_id(i, 0, 0);
			}

			return arr;
		}();
		std::vector<editor_id> _deleted_id_set[DataType_Count];
	}	 // namespace

	void reset()
	{
		for (auto vec : _deleted_id_set)
		{
			vec.clear();
		}

		for (auto i = 0; i < DataType_Count; ++i)
		{
			_next_id_vec[i] = editor_id(i, 0, 0);
		}
	}

	editor_id get_new(editor_data_type type)
	{
		if (_deleted_id_set[type].empty())
		{
			auto res		   = _next_id_vec[type];
			_next_id_vec[type] = editor_id(type, 0, (res.key() + 1));
			return res;
		}
		else
		{
			auto res = _deleted_id_set[type].back();
			res.increase_gen();

			_deleted_id_set[type].pop_back();
			return res;
		}
	}

	void delete_id(editor_id id)
	{
		if (std::ranges::find(_deleted_id_set[id.type()], id) == _deleted_id_set[id.type()].end())
		{
			_deleted_id_set[id.type()].push_back(id);
		}
	}

	void restore(editor_id id)
	{
		auto it = std::ranges::find(_deleted_id_set[id.type()], id);
		assert(it != _deleted_id_set[id.type()].end());
		_deleted_id_set[id.type()].erase(it);
	}
}	 // namespace editor::id

namespace editor::undoredo
{
	namespace
	{
		std::vector<undo_redo_cmd> _undo_vec;
		std::vector<undo_redo_cmd> _redo_vec;

		editor_command _cmd_undo {
			"Undo",
			ImGuiKey_Z | ImGuiKey_ModCtrl,
			[](editor_id _) {
				return _undo_vec.empty() is_false;
			},
			[](editor_id _) {
				auto command = _undo_vec.back();
				_undo_vec.pop_back();
				command.undo();
				_redo_vec.emplace_back(command);

				editor::logger::info(std::format("undo command : {}", command.name));
			}
		};
		editor_command _cmd_redo {
			"Redo",
			ImGuiKey_Z | ImGuiKey_ModCtrl | ImGuiKey_ModShift,
			[](editor_id _) {
				return _redo_vec.empty() is_false;
			},
			[](editor_id _) {
				auto command = _redo_vec.back();
				_redo_vec.pop_back();
				command.redo();
				_undo_vec.emplace_back(command);

				editor::logger::info(std::format("redo command : {}", command.name));
			}
		};
	}	 // namespace

	void on_project_loaded()
	{
		_redo_vec.clear();
		_undo_vec.clear();

		auto res  = editor::add_context_item("Main Menu\\Edit\\Undo", &_cmd_undo);
		res		 &= editor::add_context_item("Main Menu\\Edit\\Redo", &_cmd_redo);
		assert(res);
	}

	void add(const undo_redo_cmd& undo_redo)
	{
		_undo_vec.emplace_back(undo_redo);
		_redo_vec.clear();
	}

	void print_all()
	{
		logger::info("Redo----------------------------");
		for (auto& redo : _redo_vec)
		{
			logger::info("Redo : {}", redo.name);
		}

		logger::info("Undo----------------------------");

		for (auto& undo : _undo_vec)
		{
			logger::info("Undo : {}", undo.name);
		}
		logger::info("----------------------------");
	}
}	 // namespace editor::undoredo

namespace editor::models
{
	em_component::~em_component()
	{
		if (need_cleanup)
		{
			free(p_value);
		}
	}

	em_component::em_component(const em_component& other) : id(other.id), struct_id(other.struct_id), entity_id(other.entity_id), need_cleanup(other.need_cleanup)
	{
		if (need_cleanup)
		{
			auto size = reflection::find_struct(struct_id)->size;
			p_value	  = malloc(size);
			memcpy(p_value, other.p_value, size);
		}
	}

	em_component& em_component::operator=(const em_component& other)
	{
		this->~em_component();

		id			 = other.id;
		struct_id	 = other.struct_id;
		entity_id	 = other.entity_id;
		need_cleanup = other.need_cleanup;

		if (need_cleanup)
		{
			auto size = reflection::find_struct(other.struct_id)->size;
			p_value	  = malloc(size);
			memcpy(p_value, other.p_value, size);
		}

		return *this;
	}

	em_component::em_component(em_component&& other) noexcept : id(other.id), struct_id(other.struct_id), entity_id(other.entity_id), need_cleanup(other.need_cleanup), p_value(other.p_value)
	{
		other.p_value	   = nullptr;
		other.need_cleanup = false;
	}

	em_component& em_component::operator=(em_component&& other) noexcept
	{
		this->~em_component();
		*this = { other };
		return *this;
	}

	namespace reflection
	{
		namespace
		{
			std::unordered_map<editor_id, uint64, editor_id::hash_func>					   _struct_idx_map;
			std::unordered_map<editor_id, std::pair<uint64, uint64>, editor_id::hash_func> _field_idx_map;

			std::vector<em_struct>			   _structs;
			std::vector<std::vector<em_field>> _fields;
		}	 // namespace

		em_struct* find_struct(editor_id id)
		{
			if (_struct_idx_map.contains(id) is_false)
			{
				return nullptr;
			}

			return &_structs[_struct_idx_map[id]];
		}

		em_struct* find_struct(const char* name)
		{
			auto res = std::ranges::find_if(
				_structs, [=](em_struct s) { return s.name == name; });
			return &_structs[std::distance(_structs.begin(), res)];
		}

		editor_id create_struct()
		{
			auto  id = id::get_new(DataType_Struct);
			auto& s	 = _structs.emplace_back();
			_fields.emplace_back();
			_struct_idx_map.insert({ id, _structs.size() - 1 });
			s.id = id;

			return id;
		}

		void remove_struct(editor_id struct_id)
		{
			if (_struct_idx_map.contains(struct_id) is_false)
			{
				return;
			}

			auto idx						  = _struct_idx_map[struct_id];
			_structs[idx]					  = _structs.back();
			_struct_idx_map[_structs[idx].id] = idx;
			_structs.pop_back();
			_struct_idx_map.erase(struct_id);

			std::ranges::for_each(_fields[idx], [](em_field& f) { remove_field(f.id); });

			_fields[idx] = _fields.back();
			_fields.pop_back();

			std::ranges::for_each(_fields[idx], [=](em_field& f) { _field_idx_map[f.id].first = idx; });

			id::delete_id(struct_id);
		}

		editor_id add_field(editor_id struct_id)
		{
			auto  id		 = id::get_new(DataType_Field);
			auto  struct_idx = _struct_idx_map[struct_id];
			auto& f			 = _fields[struct_idx].emplace_back();
			f.id			 = id;
			f.struct_id		 = struct_id;
			_field_idx_map.insert({ id, { struct_idx, _fields[struct_idx].size() - 1 } });

			return id;
		}

		void remove_field(editor_id field_id)
		{
			auto& pair	 = _field_idx_map[field_id];
			auto  s_idx	 = pair.first;
			auto  f_idx	 = pair.second;
			auto& f		 = _fields[f_idx];
			auto& f_vec	 = _fields[s_idx];
			f_vec[f_idx] = f_vec.back();
			f_vec.pop_back();

			_field_idx_map.erase(field_id);
			_field_idx_map[f_vec[f_idx].id].second = f_idx;

			id::delete_id(field_id);
		}

		std::vector<em_struct*> all_structs()
		{
			return std::ranges::to<std::vector>(_structs | std::views::transform([](em_struct& s) { return &s; }));
		}

		std::vector<em_field*> all_fields(editor_id struct_id)
		{
			return std::ranges::to<std::vector>(_fields[_struct_idx_map[struct_id]] | std::views::transform([](em_field& f) { return &f; }));
		}

		em_field* find_field(editor_id id)
		{
			if (_field_idx_map.contains(id) is_false)
			{
				return nullptr;
			}

			auto& pair = _field_idx_map[id];
			return &_fields[pair.first][pair.second];
		}

		std::vector<em_field*> find_fields(std::vector<editor_id> struct_id_vec)
		{
			auto view = struct_id_vec | std::views::filter([](auto id) { return _field_idx_map.contains(id); }) | std::views::transform([](auto id) {
							auto& pair = _field_idx_map[id];
							return &_fields[pair.first][pair.second];
						});

			return std::vector<em_field*>(view.begin(), view.end());	// c++ 23 => std::views::to
		}

		void on_project_unloaded()
		{
			_struct_idx_map.clear();
			_field_idx_map.clear();
			_structs.clear();
			_fields.clear();
		}

		void on_project_loaded()
		{
			auto res = true;
			// res		 &= add_context_item("Scene\\Add New World", &editor::models::world::cmd_create);
			// res		 &= add_context_item("World\\Remove World", &editor::models::world::cmd_remove);
			assert(res);
		}
	}	 // namespace reflection

	namespace reflection::utils
	{
		namespace
		{
			struct type_info
			{
				const char* name;
				size_t		size;
			};

			type_info _info_lut[primitive_type_count] {
				{ "int2", sizeof(int2) },
				{ "int3", sizeof(int3) },
				{ "int4", sizeof(int4) },

				{ "uint2", sizeof(uint2) },
				{ "uint3", sizeof(uint3) },
				{ "uint4", sizeof(uint4) },

				{ "float2", sizeof(float2) },
				{ "float2a", sizeof(float2a) },
				{ "float3", sizeof(float3) },
				{ "float3a", sizeof(float3a) },
				{ "float4", sizeof(float4) },
				{ "float4a", sizeof(float4a) },

				{ "float3x3", sizeof(float3x3) },
				{ "float4x4", sizeof(float4x4) },
				{ "float4x4a", sizeof(float4x4a) },

				{ "uint64", sizeof(uint64) },
				{ "uint32", sizeof(uint32) },
				{ "uint16", sizeof(uint16) },
				{ "uint8", sizeof(uint8) },

				{ "int64", sizeof(int64) },
				{ "int32", sizeof(int32) },
				{ "int16", sizeof(int16) },
				{ "int8", sizeof(int8) },

				{ "float32", sizeof(float32) },
				{ "double64", sizeof(double64) },
			};
		}	 // namespace

		size_t type_size(e_primitive_type type)
		{
			return _info_lut[type].size;
		}

		const char* type_to_string(e_primitive_type type)
		{
			return _info_lut[type].name;
		}

		std::string deserialize(e_primitive_type type, const void* ptr)
		{
			switch (type)
			{
			case primitive_type_int2:
				return std::format("{}, {}", ((int2*)ptr)->x, ((int2*)ptr)->y);
			case primitive_type_int3:
				return std::format("{}, {}, {}", ((int3*)ptr)->x, ((int3*)ptr)->y, ((int3*)ptr)->z);
			case primitive_type_int4:
				return std::format("{}, {}, {}, {}", ((int4*)ptr)->x, ((int4*)ptr)->y, ((int4*)ptr)->z, ((int4*)ptr)->w);
			case primitive_type_uint2:
				return std::format("{}, {}", ((uint2*)ptr)->x, ((uint2*)ptr)->y);
			case primitive_type_uint3:
				return std::format("{}, {}, {}", ((uint3*)ptr)->x, ((uint3*)ptr)->y, ((uint3*)ptr)->z);
			case primitive_type_uint4:
				return std::format("{}, {}, {}, {}", ((uint4*)ptr)->x, ((uint4*)ptr)->y, ((uint4*)ptr)->z, ((uint4*)ptr)->w);

			case primitive_type_float2:
				return std::format("{:.5f}, {:.5f}", ((float2*)ptr)->x, ((float2*)ptr)->y);
			case primitive_type_float2a:
				return std::format("{:.5f}, {:.5f}", ((float2a*)ptr)->x, ((float2a*)ptr)->y);
			case primitive_type_float3:
				return std::format("{:.5f}, {:.5f}, {:.5f}", ((float3*)ptr)->x, ((float3*)ptr)->y, ((float3*)ptr)->z);
			case primitive_type_float3a:
				return std::format("{:.5f}, {:.5f}, {:.5f}", ((float3a*)ptr)->x, ((float3a*)ptr)->y, ((float3a*)ptr)->z);
			case primitive_type_float4:
				return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}", ((float4*)ptr)->x, ((float4*)ptr)->y, ((float4*)ptr)->z, ((float4*)ptr)->w);
			case primitive_type_float4a:
				return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}", ((float4a*)ptr)->x, ((float4a*)ptr)->y, ((float4a*)ptr)->z, ((float4a*)ptr)->w);

			case primitive_type_float3x3:
				return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}",
								   ((float3x3*)ptr)->_11, ((float3x3*)ptr)->_12, ((float3x3*)ptr)->_13,
								   ((float3x3*)ptr)->_21, ((float3x3*)ptr)->_22, ((float3x3*)ptr)->_23,
								   ((float3x3*)ptr)->_31, ((float3x3*)ptr)->_32, ((float3x3*)ptr)->_33);
			case primitive_type_float4x4:
			case primitive_type_float4x4a:
				return std::format("{:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}, {:.5f}",
								   ((float4x4a*)ptr)->_11, ((float4x4a*)ptr)->_12, ((float4x4a*)ptr)->_13, ((float4x4a*)ptr)->_14,
								   ((float4x4a*)ptr)->_21, ((float4x4a*)ptr)->_22, ((float4x4a*)ptr)->_23, ((float4x4a*)ptr)->_24,
								   ((float4x4a*)ptr)->_31, ((float4x4a*)ptr)->_32, ((float4x4a*)ptr)->_33, ((float4x4a*)ptr)->_34,
								   ((float4x4a*)ptr)->_41, ((float4x4a*)ptr)->_42, ((float4x4a*)ptr)->_43, ((float4x4a*)ptr)->_44);
			case primitive_type_uint64:
				return std::format("{}", *(uint64*)ptr);
			case primitive_type_uint32:
				return std::format("{}", *(uint32*)ptr);
			case primitive_type_uint16:
				return std::format("{}", *(uint16*)ptr);
			case primitive_type_uint8:
				return std::format("{}", *(uint8*)ptr);
			case primitive_type_int64:
				return std::format("{}", *(int64*)ptr);
			case primitive_type_int32:
				return std::format("{}", *(int32*)ptr);
			case primitive_type_int16:
				return std::format("{}", *(int16*)ptr);
			case primitive_type_int8:
				return std::format("{}", *(int8*)ptr);
			case primitive_type_float32:
				return std::format("{}", *(float32*)ptr);
			case primitive_type_double64:
				return std::format("{}", *(double64*)ptr);
			default:
				break;
			}

			// todo nested
			assert(false);
			return {};
		}

		std::vector<std::string> deserialize(editor_id struct_id)
		{
			auto p_s = reflection::find_struct(struct_id);
			return std::ranges::to<std::vector>(
				reflection::all_fields(struct_id)
				| std::views::transform([=](auto* p_field) { return deserialize(p_field->type, p_s->p_default_value); }));
		}

		std::vector<std::string> deserialize(editor_id struct_id, const void* ptr)
		{
			auto p_s = reflection::find_struct(struct_id);
			return std::ranges::to<std::vector>(
				reflection::all_fields(struct_id)
				| std::views::transform([=](auto* p_field) { return deserialize(p_field->type, ptr); }));
		}
	}	 // namespace reflection::utils

	namespace scene
	{
		// todo reserve order after delete, insert => set
		namespace
		{
			// std::vector<em_scene>										_scenes;
			// std::unordered_map<editor_id, uint32, editor_id::hash_func> _idx_map;

			std::map<editor_id, em_scene> _scenes;	  // does not support reordering -> change id?
			editor_id					  _current;
		}											  // namespace

		em_scene* find(editor_id id)
		{
			if (_scenes.contains(id))
			{
				return &_scenes[id];
			}
			else
			{
				return nullptr;
			}
		}

		editor_id create()
		{
			auto id = id::get_new(DataType_Scene);

			auto& s	 = _scenes[id];
			s.name	 = std::format("new_scene_{0}", s.id.str());
			s.id	 = id;
			_current = id;

			return s.id;
		}

		void remove(editor_id id)
		{
			_scenes.erase(id);
			id::delete_id(id);
		}

		size_t count()
		{
			return _scenes.size();
		}

		std::vector<em_scene*> all()
		{
			return std::ranges::to<std::vector>(_scenes | std::views::transform([](auto&& pair) { return &pair.second; }));
		}

		void set_current(editor_id id)
		{
			if (find(id))
			{
				_current = id;
				editor::select_new(_current);
			}
			else
			{
				assert(false);
			}
		}

		em_scene* get_current()
		{
			auto p_s = find(_current);
			if (p_s is_nullptr)
			{
				return &(_scenes.begin()->second);
			}
			else
			{
				return p_s;
			}
		}

		editor_command cmd_create(
			"New Scene",
			ImGuiKey_N | ImGuiMod_Ctrl,
			[](editor_id _) { return true; },
			[](editor_id _) {
				auto cmd			= undoredo::undo_redo_cmd();
				auto backup_current = _current;
				cmd.name			= "new scene";
				cmd.redo			= []() { set_current(create()); };
				cmd.undo			= [=]() {
					   remove(_scenes.rbegin()->first);
					   set_current(backup_current);
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_remove(
			"Delete Scene",
			ImGuiKey_Delete,
			[](editor_id _) {
				return count() - get_all_selections().size() > 0 and std::ranges::all_of(get_all_selections(), [](editor_id id) { return find(id) != nullptr; });
			},
			[](editor_id _) {
				auto cmd			= undoredo::undo_redo_cmd();
				auto id_vec			= editor::get_all_selections();
				auto backup_current = _current;
				auto backup_vec		= std::ranges::to<std::vector>(
					editor::get_all_selections()
					| std::views::transform([](editor_id id) { return *find(id); }));

				cmd.name = "delete scene";
				cmd.redo = [=]() {
					for (const auto s_id : id_vec)
					{
						remove(s_id);
					}
				};
				cmd.undo = [=]() {
					for (const auto& s : backup_vec | std::views::reverse)
					{
						_scenes[s.id] = s;
						id::restore(s.id);
					}

					_current = backup_current;
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_set_current(
			"Set Current",
			ImGuiKey_None,
			[](editor_id id) { return id != _current and find(id) != nullptr; },
			[](editor_id id) {
				auto cmd			= undoredo::undo_redo_cmd();
				auto backup_current = _current;

				cmd.name = std::format("set current scene from {} to {}", backup_current.str(), id.str());
				cmd.undo = [=]() { _current = backup_current; };
				cmd.redo = [=]() { _current = id; };

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		void on_project_unloaded()
		{
			_scenes.clear();
		}

		void on_project_loaded()
		{
			auto res  = true;
			res		 &= add_context_item("Scene\\Add New Scene", &scene::cmd_create);
			res		 &= add_context_item("Scene\\Remove Scene", &scene::cmd_remove);
			assert(res);
		}
	}	 // namespace scene

	namespace world
	{
		namespace
		{
			std::unordered_map<editor_id, std::map<editor_id, em_world>, editor_id::hash_func> _worlds;			 // key : scene_id , value : [key : world_id, value : em_world]
			std::unordered_map<editor_id, editor_id, editor_id::hash_func>					   _scene_id_lut;	 // key : world_id, value : scene_id
																												 // _worlds => [scene_idx][world_idx]
																												 // _idx_map => [world_id] [pair<scene_idx, world_idx>]

			em_world* _find(editor_id s_id, editor_id w_id)
			{
				assert(_worlds[s_id].contains(w_id));
				return &_worlds[s_id][w_id];
			}

		}	 // namespace

		em_world* find(editor_id w_id)
		{
			if (_scene_id_lut.contains(w_id))
			{
				return _find(_scene_id_lut[w_id], w_id);
			}
			else
			{
				return nullptr;
			}
		}

		editor_id create(editor_id scene_id)
		{
			assert(scene::find(scene_id) != nullptr);

			auto  w_id			= id::get_new(DataType_World);
			auto& w				= _worlds[scene_id][w_id];
			w.id				= w_id;
			w.name				= std::format("new_world_{}", w_id.str());
			w.scene_id			= scene_id;
			_scene_id_lut[w.id] = scene_id;
			return w.id;
		}

		void add_struct(editor_id world_id, editor_id struct_id)
		{
			// todo
			// problem 1 : archetype ordering => based on hash_id => solved
			// problem 2 : cannot select struct => what is the name of this struct? component? struct? archetype? => add remove struct from world
			// problem 3 : duplications => solved
			auto* p_w = find(world_id);
			if (p_w is_nullptr or p_w->structs.size() == 64 /*or std::ranges::find(p_w->structs, struct_id) != p_w->structs.end()*/)
			{
				return;
			}

			p_w->structs.insert(std::ranges::upper_bound(p_w->structs, struct_id,
														 [](const auto& comp_id, const auto id) { return reflection::find_struct(comp_id)->hash_id < reflection::find_struct(id)->hash_id; }),
								struct_id);
		}

		void remove_struct(editor_id world_id, editor_id struct_id)
		{
			auto* p_w = find(world_id);
			std::erase(p_w->structs, struct_id);
		}

		uint64 archetype(editor_id world_id, editor_id struct_id)
		{
			auto p_w = find(world_id);
			if (p_w is_nullptr)
			{
				return 0;
			}

			auto it = std::ranges::find(p_w->structs, struct_id);

			if (it == p_w->structs.end())
			{
				return 0;
			}

			auto index = it - p_w->structs.begin();
			assert(index < 64);
			return 1 << index;
		}

		void remove(editor_id world_id)
		{
			if (_scene_id_lut.contains(world_id) is_false)
			{
				return;
			}

			auto scene_id = _scene_id_lut[world_id];
			_worlds[scene_id].erase(world_id);
			_scene_id_lut.erase(world_id);
			id::delete_id(world_id);
		}

		std::vector<em_world*> all(editor_id scene_id)
		{
			assert(scene::find(scene_id) != nullptr);

			return std::ranges::to<std::vector>(_worlds[scene_id]
												| std::views::values
												| std::views::transform([](auto&& w) { return &w; }));
		}

		editor_command cmd_create(
			"New World",
			ImGuiKey_None,
			[](editor_id _) { return editor::get_current_selection().type() == DataType_Scene and editor::get_all_selections().size() == 1; },
			[](editor_id _) {
				auto cmd  = undoredo::undo_redo_cmd();
				auto s_id = editor::get_current_selection();
				cmd.name  = "new world";
				cmd.redo  = [=]() { create(s_id); };
				cmd.undo  = [=]() { remove(_worlds[s_id].rbegin()->first); };

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_remove(
			"Remove World",
			ImGuiKey_Delete,
			[](editor_id _) { return editor::get_all_selections().empty() is_false and std::ranges::all_of(editor::get_all_selections(), [](editor_id id) { return find(id) != nullptr; }); },
			[](editor_id _) {
				auto  cmd	 = undoredo::undo_redo_cmd();
				auto& id_vec = editor::get_all_selections();

				auto backup_vec = std::ranges::to<std::vector>(editor::get_all_selections()
															   | std::views::transform([](auto world_id) { return *find(world_id); }));

				cmd.name = "remove world";
				cmd.redo = [=]() {
					std::ranges::for_each(id_vec, [](editor_id id) {
						remove(id);
					});
				};

				cmd.undo = [=]() {
					for (auto&& w : backup_vec | std::views::reverse)
					{
						_scene_id_lut[w.id]		  = w.scene_id;
						_worlds[w.scene_id][w.id] = w;
						id::restore(w.id);
					}
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_add_struct(
			"Add Struct",
			ImGuiKey_None,
			[](editor_id struct_id) { return reflection::find_struct(struct_id) != nullptr and std::ranges::all_of(get_all_selections(), [=](const auto& id) {
												 const auto* p_w = world::find(id);
												 return p_w	 is_not_nullptr and std::ranges::find(p_w->structs, struct_id) == p_w->structs.end();
											 }); },
			[](editor_id struct_id) {
				auto  cmd	 = undoredo::undo_redo_cmd();
				auto  p_s	 = reflection::find_struct(struct_id);
				auto& id_vec = get_all_selections();

				cmd.name = "add struct";
				cmd.redo = [=]() {
					std::ranges::for_each(id_vec, [=](const auto& id) {
						world::add_struct(id, struct_id);
					});
				};
				cmd.undo = [=]() {
					std::ranges::for_each(id_vec | std::views::reverse, [=](const auto& id) {
						remove_struct(id, struct_id);
					});
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_remove_struct(
			"Remove Struct",
			ImGuiKey_None,
			// todo check that no entities uses that struct
			[](editor_id struct_id) { return reflection::find_struct(struct_id) is_not_nullptr
										 and std::ranges::all_of(get_all_selections(), [=](const auto& w_id) {
												 return std::ranges::all_of(entity::all(w_id), [=](em_entity* p_e) {
													 auto arc = archetype(w_id, struct_id);
													 return arc != 0 and ((p_e->archetype & arc) == 0);
												 });
											 }); },
			[](editor_id struct_id) {
				auto  cmd	 = undoredo::undo_redo_cmd();
				auto  p_s	 = reflection::find_struct(struct_id);
				auto& id_vec = get_all_selections();

				cmd.name = "remove struct";
				cmd.redo = [=]() {
					std::ranges::for_each(id_vec, [=](const auto& id) {
						world::remove_struct(id, struct_id);
					});
				};
				cmd.undo = [=]() {
					std::ranges::for_each(id_vec | std::views::reverse, [=](const auto& id) {
						add_struct(id, struct_id);
					});
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		void on_project_unloaded()
		{
			_worlds.clear();
			_scene_id_lut.clear();
		}

		void on_project_loaded()
		{
			auto res  = true;
			res		 &= add_context_item("Scene\\Add New World", &world::cmd_create);
			res		 &= add_context_item("World\\Remove World", &world::cmd_remove);

			std::ranges::for_each(reflection::all_structs(), [&](const em_struct* p_s) {
				res &= add_context_item(std::format("World\\Add Struct\\{}", p_s->name), &cmd_add_struct, p_s->id);
				res &= add_context_item(std::format("World\\Remove Struct\\{}", p_s->name), &cmd_remove_struct, p_s->id);
			});
			assert(res);
		}
	}	 // namespace world

	namespace entity
	{
		namespace
		{
			std::unordered_map<editor_id, std::map<editor_id, em_entity>, editor_id::hash_func> _entities;		  // key : world_id, value : map [  entity_id, em_entity ]
			std::unordered_map<editor_id, editor_id, editor_id::hash_func>						_world_id_lut;	  // key : entity id, value : world_id
		}																										  // namespace

		em_entity* find(editor_id entity_id)
		{
			if (_world_id_lut.contains(entity_id))
			{
				auto world_id = _world_id_lut[entity_id];
				assert(world::find(world_id));

				return &_entities[world_id][entity_id];
			}
			else
			{
				return nullptr;
			}
		}

		editor_id create(editor_id world_id)
		{
			assert(world::find(world_id));

			auto entity_id = id::get_new(DataType_Entity);

			{
				auto& e	   = _entities[world_id][entity_id];
				e.id	   = entity_id;
				e.name	   = std::format("new_entity");
				e.world_id = world_id;
			}

			_world_id_lut[entity_id] = world_id;
			return entity_id;
		}

		void remove(editor_id entity_id)
		{
			if (_world_id_lut.contains(entity_id) is_false)
			{
				return;
			}

			auto world_id = _world_id_lut[entity_id];
			_entities[world_id].erase(entity_id);
			_world_id_lut.erase(entity_id);
			id::delete_id(entity_id);
		}

		std::vector<em_entity*> all(editor_id world_id)
		{
			assert(world::find(world_id) != nullptr);

			return std::ranges::to<std::vector>(_entities[world_id] | std::views::transform([](auto&& pair) { return &pair.second; }));
		}

		editor_command cmd_create_empty(
			"Create Emtpy Entity",
			ImGuiKey_None,
			[](editor_id _) { return std::ranges::all_of(
								  editor::get_all_selections(),
								  [](const auto id) { return id.type() == DataType_World and world::find(id) != nullptr; }); },
			[](editor_id _) {
				auto cmd = undoredo::undo_redo_cmd();

				auto selections = editor::get_all_selections();

				cmd.name = "create empty entity";
				cmd.redo = [=]() {
					std::ranges::for_each(selections, create);
				};

				cmd.undo = [=]() {
					std::ranges::for_each(selections
											  | std::views::transform([](auto world_id) { return *_entities[world_id].rbegin(); })
											  | std::views::keys
											  | std::views::reverse,
										  remove);
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_remove(
			"Remove Entities",
			ImGuiKey_Delete,
			[](editor_id _) { return std::ranges::all_of(
								  editor::get_all_selections(),
								  [](const auto id) { return id.type() == DataType_Entity and entity::find(id) != nullptr; }); },
			[](editor_id _) {
				auto cmd		= undoredo::undo_redo_cmd();
				auto selections = editor::get_all_selections();
				auto backup_vec = std::ranges::to<std::vector>(
					editor::get_all_selections()
					| std::views::transform([](editor_id entity_id) { return *find(entity_id); }));

				cmd.name = "remove entities";
				cmd.redo = [=]() {
					std::ranges::for_each(selections, remove);
				};

				cmd.undo = [=]() {
					for (auto&& e : backup_vec | std::views::reverse)
					{
						_entities[e.world_id][e.id] = e;
						_world_id_lut[e.id]			= e.world_id;
					}
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		editor_command cmd_add_component(
			"Add Component",
			ImGuiKey_None,
			[](editor_id struct_id) { return std::ranges::all_of(
										  editor::get_all_selections() | std::views::transform(entity::find),
										  [=](auto* p_e) { return world::archetype(p_e->world_id, struct_id) != 0 and component::find(p_e->id, struct_id) is_nullptr; }); },
			[](editor_id struct_id) {
				auto  cmd		 = undoredo::undo_redo_cmd();
				auto& selections = editor::get_all_selections();
				auto  backup_vec = std::ranges::to<std::vector>(selections
																| std::views::transform([=](auto e_id) { return component::find(e_id, struct_id); }));

				cmd.name = "add component";
				cmd.redo = [=]() {
					std::ranges::for_each(selections, [=](auto e_id) { component::create(e_id, struct_id); });
				};

				cmd.undo = [=]() {
					std::ranges::for_each(selections, [=](auto e_id) { component::remove(component::find(e_id, struct_id)->id); });
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		void on_project_unloaded()
		{
			_entities.clear();
			_world_id_lut.clear();
		}

		void on_project_loaded()
		{
			auto res = true;
			// res		 &= add_context_item("Entity\\Add New Entity", &entity::cmd_create);
			// res		 &= add_context_item("Entity\\Remove Entity", &entity::cmd_remove);
			res &= add_context_item("World\\Entity\\Create Empty", &entity::cmd_create_empty);
			res &= add_context_item("Entity\\Remove Entity", &entity::cmd_remove);

			std::ranges::for_each(reflection::_structs, [&res](auto&& s) {
				res &= add_context_item(std::format("Entity\\Add Component\\{}", s.name), &cmd_add_component, s.id);
			});


			assert(res);
		}
	}	 // namespace entity

	namespace component
	{
		namespace
		{
			std::unordered_map<editor_id, std::vector<em_component>, editor_id::hash_func> _components;		  // key : endity_id, value : [key : component_id, value : em_component]
			std::unordered_map<editor_id, editor_id, editor_id::hash_func>				   _entity_id_lut;	  // key: component_id, value: entity_id
		}																									  // namespace

		em_component* find(editor_id component_id)
		{
			if (_entity_id_lut.contains(component_id))
			{
				auto e_id = _entity_id_lut[component_id];
				auto res  = std::ranges::find_if(_components[e_id], [=](auto&& c) { return c.id == component_id; });
				if (res != _components[e_id].end())
				{
					return &(*res);
				}
			}

			return nullptr;
		}

		em_component* find(editor_id entity_id, editor_id struct_id)
		{
			if (_components.contains(entity_id))
			{
				auto res = std::ranges::find_if(_components[entity_id], [=](auto&& c) { return c.struct_id == struct_id; });
				if (res != _components[entity_id].end())
				{
					return &(*res);
				}
			}

			return nullptr;
		}

		editor_id create(editor_id entity_id, editor_id struct_id, void* p_value)
		{
			auto* p_s = reflection::find_struct(struct_id);
			assert(entity::find(entity_id));
			assert(p_s);

			auto&& em_c = em_component();

			em_c.id		   = id::get_new(DataType_Component);
			em_c.entity_id = entity_id;
			em_c.struct_id = struct_id;
			if (p_value is_not_nullptr)
			{
				em_c.need_cleanup = false;
				em_c.p_value	  = p_value;
			}
			else
			{
				em_c.need_cleanup = true;
				em_c.p_value	  = malloc(p_s->size);
				memcpy(em_c.p_value, p_s->p_default_value, p_s->size);
			}


			_entity_id_lut[em_c.id] = entity_id;

			auto it = _components[entity_id].insert(std::ranges::upper_bound(_components[entity_id], struct_id, std::ranges::less {}, &em_component::struct_id), std::move(em_c));
			return em_c.id;
		}

		void remove(editor_id component_id)
		{
			if (_entity_id_lut.contains(component_id))
			{
				auto e_id = _entity_id_lut[component_id];
				_components[e_id].erase(std::ranges::find_if(_components[e_id], [=](auto&& c) { return c.id == component_id; }));
				_entity_id_lut.erase(component_id);

				id::delete_id(component_id);
			}
		}

		std::vector<em_component*> all(editor_id entity_id)
		{
			assert(entity::find(entity_id) is_not_nullptr);

			return std::ranges::to<std::vector>(_components[entity_id] | std::views::transform([](em_component& s) { return &s; }));
		}

		editor_command cmd_remove_component(
			"Add Component",
			ImGuiKey_None,
			[](editor_id _) { return std::ranges::all_of(editor::get_all_selections(), [](auto c_id) { return find(c_id) is_not_nullptr; }); },
			[](editor_id _) {
				auto  cmd		 = undoredo::undo_redo_cmd();
				auto& selections = editor::get_all_selections();

				auto backup_vec = std::ranges::to<std::vector>(
					editor::get_all_selections()
					| std::views::transform([](auto id) { return *find(id); }));

				cmd.name = "remove component";
				cmd.redo = [=]() {
					std::ranges::for_each(selections, remove);
				};

				cmd.undo = [=]() {
					std::ranges::for_each(backup_vec, [=](auto&& c) {
						id::restore(c.id);

						component::_entity_id_lut[c.id] = c.entity_id;

						component::_components[c.entity_id].insert(
							std::ranges::upper_bound(_components[c.entity_id], c.struct_id, std::ranges::less {}, &em_component::struct_id), c);
					});
				};

				undoredo::add(cmd);
				logger::info(cmd.name);
				cmd.redo();
			});

		void on_project_unloaded()
		{
			_components.clear();
			_entity_id_lut.clear();
		}

		void on_project_loaded()
		{
			auto res  = true;
			res		 &= add_context_item("Component\\Remove Component", &cmd_remove_component);

			assert(res);
		}

		// namespace
	}	 // namespace component

	namespace text
	{
		namespace
		{
			std::unordered_map<editor_id, const char*, editor_id::hash_func> _text_map;
		}

		editor_id create(const char* p_text)
		{
			auto id = id::get_new(DataType_Editor_Text);
			_text_map.insert({ id, p_text });
			return id;
		}

		void remove(editor_id id)
		{
			_text_map.erase(id);
		}

		const char* find(editor_id id)
		{
			return _text_map[id];
		}
	}	 // namespace text

	editor_command cmd_rename_selection(
		"Rename",
		ImGuiKey_None,
		// todo check that no entities uses that struct
		[](editor_id text_id) { return text_id.type() == DataType_Editor_Text and find(get_current_selection()); },
		[](editor_id text_id) {
			//  todo
			//  change editor command signature to take void*
			//  or create editor object with corresponding datatype and editor_id (ex. editor::models::text::create(const char* )
			//  const char* new_text = editor::models::text::find(text_id);

			// todo maybe another way to to do this
			auto id			= editor::get_current_selection();
			auto backup_str = *get_name(id);
			auto text_str	= std::string(text::find(text_id));

			auto cmd = undoredo::undo_redo_cmd();

			cmd.name = std::format("rename {} from {} to {}", id.value, *get_name(id), text_str);
			cmd.undo = [=]() { *get_name(id) = backup_str; };
			cmd.redo = [=]() { *get_name(id) = text_str; };
			undoredo::add(cmd);
			logger::info(cmd.name);
			cmd.redo();

			text::remove(text_id);
		});

	void on_project_unloaded()
	{
		reflection::on_project_unloaded();
		scene::on_project_unloaded();
		world::on_project_unloaded();
		entity::on_project_unloaded();
		component::on_project_unloaded();
		// component::on_project_unloaded();
	}

	void* find(editor_id id)
	{
		switch (id.type())
		{
		case DataType_Entity:
			return entity::find(id);
		case DataType_Project:
			return nullptr;
		case DataType_Scene:
			return scene::find(id);
		case DataType_World:
			return world::find(id);
		case DataType_SubWorld:
			return nullptr;
		case DataType_Component:
			return component::find(id);
		case DataType_System:
			return nullptr;
		case DataType_Struct:
			return reflection::find_struct(id);
		case DataType_Field:
			return reflection::find_field(id);
		case DataType_Editor_Text:
			return (void*)text::find(id);
		case DataType_Editor_Command:
		case DataType_Editor_UndoRedo:
		case DataType_Count:
		case DataType_InValid:
			return nullptr;
		}

		return nullptr;
	}

	std::string* get_name(editor_id id)
	{
		switch (id.type())
		{
		case DataType_Entity:
		{
			auto* ptr = entity::find(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_Project:
			return nullptr;
		case DataType_Scene:
		{
			auto* ptr = scene::find(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_World:
		{
			auto* ptr = world::find(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_SubWorld:
			return nullptr;
		case DataType_Component:
		case DataType_System:
			return nullptr;
		case DataType_Struct:
		{
			auto* ptr = reflection::find_struct(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_Field:
		{
			auto* ptr = reflection::find_field(id);
			return ptr is_nullptr ? nullptr : &ptr->name;
		}
		case DataType_Editor_Text:
		case DataType_Editor_Command:
		case DataType_Editor_UndoRedo:
		case DataType_Count:
		case DataType_InValid:
			return nullptr;
		}

		return nullptr;
	}

	bool change_exists()
	{
		return true;
	}

	void on_project_loaded()
	{
		reflection::on_project_loaded();
		scene::on_project_loaded();
		world::on_project_loaded();
		entity::on_project_loaded();
		component::on_project_loaded();
	}
}	 // namespace editor::models

// commands
namespace editor
{
	bool is_selected(editor_id id)
	{
		return std::find(_selected_vec.begin(), _selected_vec.end(), id) != _selected_vec.end();
	}

	void select_new(editor_id id)
	{
		_selected_vec.clear();
		_selected_vec.push_back(id);
	}

	void add_select(editor_id id)
	{
		_selected_vec.push_back(id);
	}

	void deselect(editor_id id)
	{
		_selected_vec.erase(std::ranges::find(_selected_vec, id));
	}

	void add_right_click_source(editor_id id)
	{
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) and widgets::is_item_hovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
		{
			auto it = std::find(_selected_vec.begin(), _selected_vec.end(), id);
			if (it == _selected_vec.end())
			{
				select_new(id);
			}
			else
			{
				std::iter_swap(it, _selected_vec.rbegin());
			}

			_show_ctx_menu = true;
		}
	}

	void add_left_click_source(editor_id id)
	{
		if (widgets::is_item_clicked(ImGuiMouseButton_Left))
		{
			if (platform::is_key_down(ImGuiKey_LeftCtrl))
			{
				if (is_selected(id))
				{
					cmd_deselect(id);

					// deselect(id);
				}
				else
				{
					cmd_add_select(id);
					// add_select(id);
				}
			}
			else
			{
				cmd_select_new(id);
				// select_new(id);
			}
		}
	}

	void add_left_right_click_source(editor_id id)
	{
		add_left_click_source(id);
		add_right_click_source(id);
	}

	editor_id get_current_selection()
	{
		return _selected_vec.empty() ? editor_id() : _selected_vec.back();
	}

	const std::vector<editor_id>& get_all_selections()
	{
		return _selected_vec;
	}

	bool is_selection_vec_empty()
	{
		return _selected_vec.empty();
	}

	const editor_command cmd_select_new(
		"Select new",
		ImGuiKey_None,
		[](editor_id id) { return _selected_vec.empty() or get_current_selection() != id; },
		[](editor_id id) {
			auto _selected_before = _selected_vec;
			auto cmd			  = undoredo::undo_redo_cmd();
			cmd.name			  = std::format("Select {}", id.str());
			cmd.redo			  = [id]() {
				 _selected_vec.clear();
				 _selected_vec.push_back(id);
			};
			cmd.undo = [id, _selected_before]() {
				_selected_vec = _selected_before;
			};

			undoredo::add(cmd);
			logger::info(cmd.name);
			cmd.redo();
		});

	const editor_command cmd_add_select(
		"Add Select",
		ImGuiKey_None,
		[](editor_id id) {
			return get_current_selection().type() == id.type() and std::find(_selected_vec.begin(), _selected_vec.end(), id) == _selected_vec.end();
		},
		[](editor_id id) {
			auto cmd = undoredo::undo_redo_cmd();
			cmd.name = std::format("Add Select {}", id.str());
			cmd.redo = [id]() {
				_selected_vec.push_back(id);
			};
			cmd.undo = [id]() {
				_selected_vec.pop_back();
			};

			undoredo::add(cmd);
			logger::info(cmd.name);
			cmd.redo();
		});

	const editor_command cmd_deselect(
		"Deselect",
		ImGuiKey_None,
		[](editor_id id) { return std::ranges::find(_selected_vec, id) != _selected_vec.end(); },
		[](editor_id id) {
			auto cmd   = undoredo::undo_redo_cmd();
			auto index = std::ranges::find(_selected_vec, id) - _selected_vec.begin();
			cmd.name   = std::format("Deselect {}", id.str());

			cmd.redo = [=]() {
				_selected_vec.erase(std::next(_selected_vec.begin(), index));
			};
			cmd.undo = [=]() {
				_selected_vec.insert(std::next(_selected_vec.begin(), index), id);
			};

			undoredo::add(cmd);
			logger::info(cmd.name);
			cmd.redo();
		});
}	 // namespace editor

std::string editor::utilities::read_file(const std::filesystem::path path)
{
	std::ifstream stream;
	stream.open(path);
	std::stringstream ss_project_data;
	ss_project_data << stream.rdbuf();
	return ss_project_data.str();
}

void editor::utilities::create_file(const std::filesystem::path path, const std::string content)
{
	std::ofstream project_file(path);
	project_file << content.c_str();
	project_file.close();
}
