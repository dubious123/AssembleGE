#ifndef IMGUI_DEFINE_MATH_OPERATORS
	#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dwmapi.h>

#include "editor.h"
#include "editor_view.h"
#include "logger.h"
#include "game_project\game_project.h"
#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx12.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

EditorContext* GEctx = nullptr;

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
	void Reset_Colors()
	{
		ImGuiStyle& style							 = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text]					 = COL_TEXT;
		style.Colors[ImGuiCol_TextDisabled]			 = COL_TEXT_DISABLED;
		style.Colors[ImGuiCol_WindowBg]				 = COL_BLACK;		   // COL_BG_WINDOW;
		style.Colors[ImGuiCol_ChildBg]				 = COL_BLACK;		   // COL_BG_WINDOW;	   // COL_BG_POPUP;
		style.Colors[ImGuiCol_PopupBg]				 = COL_BLACK;		   // COL_BG_WINDOW;	   // COL_BG_POPUP;
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
		style.Colors[ImGuiCol_HeaderHovered]		 = COL_GRAY_1;	  // ImVec4(0.25f, 0.25f, 0.25f, 1.00f); //selectable hovered
		style.Colors[ImGuiCol_HeaderActive]			 = COL_GRAY_2;	  // ImVec4(0.67f, 0.67f, 0.67f, 0.39f); //selectable active
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

	void Reset_Style()
	{
		ImGuiStyle& style		= ImGui::GetStyle();
		style.WindowBorderSize	= 1.0f;
		style.ChildBorderSize	= 1.0f;
		style.PopupBorderSize	= 1.0f;
		style.FrameBorderSize	= 1.0f;
		style.TabBorderSize		= 0.0f;
		style.WindowRounding	= 0.0f;
		style.ChildRounding		= 0.0f;
		style.PopupRounding		= 0.0f;
		style.FrameRounding		= 2.3f;
		style.ScrollbarRounding = 0.0f;
		style.GrabRounding		= 2.3f;
		style.TabRounding		= 0.0f;
		style.FramePadding		= ImVec2(6.f, 6.f);
		style.PopupBorderSize	= .4f;
		style.ItemSpacing.y		= 3.f;
		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		if (GImGui->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
		}
	}
}	 // namespace

void Editor::Init()
{
	// Create application window
	wc = { sizeof(wc), /*CS_CLASSDC*/ CS_HREDRAW | CS_VREDRAW | CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"AssembleGE Editor", nullptr };
	::RegisterClassExW(&wc);

	int window_style = WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE;
	ImGui_ImplWin32_EnableDpiAwareness();
	BOOL USE_DARK_MODE = true;

	float dpi_scale = ::GetDpiForSystem() / 96.f;
	HWND  hwnd		= ::CreateWindowW(wc.lpszClassName, L"AssembleGE Editor", window_style /*WS_DLGFRAME*/, 100, 100, 1280 * dpi_scale, 800 * dpi_scale, nullptr, nullptr, wc.hInstance, nullptr);

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
	GEctx			 = new EditorContext();
	GEctx->Hwnd		 = hwnd;
	GEctx->Dpi_Scale = ::GetDpiForSystem() / 96.f;

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags					|= ImGuiConfigFlags_NavEnableKeyboard;	  // Enable Keyboard Controls
	io.ConfigFlags					|= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls
	io.ConfigFlags					|= ImGuiConfigFlags_DockingEnable;		  // Enable Docking
	io.ConfigFlags					|= ImGuiConfigFlags_ViewportsEnable;	  // Enable Multi-Viewport / Platform Windows
	io.ConfigViewportsNoAutoMerge	 = true;
	io.ConfigViewportsNoTaskBarIcon	 = true;

	Reset_Colors();
	Editor::Update_Dpi_Scale(dpi_scale);

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
	bool ret = LoadTextureFromFile("AssembleGE_Icon.png", g_pd3dDevice, my_texture_srv_cpu_handle, &g_p_icon_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);

	GEctx->Icon_Texture_Id = my_texture_srv_gpu_handle.ptr;
#pragma endregion

	Editor::Logger::Init();
	Editor::View::Init();
	Editor::GameProject::Init();
}

void Editor::Update_Dpi_Scale(float new_dpi_scale)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplDX12_InvalidateDeviceObjects();

	// Setup Dear ImGui style
	ImGuiStyle& style	 = ImGui::GetStyle();
	ImGuiStyle	styleold = style;
	style				 = ImGuiStyle();
	Reset_Style();
	CopyMemory(style.Colors, styleold.Colors, sizeof(style.Colors));	// Restore colors

	style.ScaleAllSizes(new_dpi_scale);
	GEctx->Dpi_Scale = new_dpi_scale;
	io.Fonts->Clear();

	GEctx->P_Font_Arial_Default_13_5 = io.Fonts->AddFontFromFileTTF("../Resources/arial.ttf", 13.5f * GEctx->Dpi_Scale);
	GEctx->P_Font_Arial_Bold_13_5	 = io.Fonts->AddFontFromFileTTF("../Resources/arialbd.ttf", 13.5f * GEctx->Dpi_Scale);

	GEctx->P_Font_Arial_Default_16 = io.Fonts->AddFontFromFileTTF("../Resources/arial.ttf", 16.f * GEctx->Dpi_Scale);
	GEctx->P_Font_Arial_Bold_16	   = io.Fonts->AddFontFromFileTTF("../Resources/arialbd.ttf", 16.f * GEctx->Dpi_Scale);

	GEctx->P_Font_Arial_Default_18 = io.Fonts->AddFontFromFileTTF("../Resources/arial.ttf", 18.f * GEctx->Dpi_Scale);
	GEctx->P_Font_Arial_Bold_18	   = io.Fonts->AddFontFromFileTTF("../Resources/arialbd.ttf", 18.f * GEctx->Dpi_Scale);

	GImGui->Font = GEctx->P_Font_Arial_Default_13_5;

	ImGui_ImplDX12_CreateDeviceObjects();

	GEctx->Dpi_Changed = true;
	// todo
	// Editor::View::Update_Dpi_Scale();
}

void Editor::Run()
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
		if (done) break;

		COLORREF BORDER_COLOR = ::GetFocus() != NULL ? RGB(113, 96, 232) : RGB(61, 61, 61);
		::DwmSetWindowAttribute(
			(HWND)GEctx->Hwnd, DWMWINDOWATTRIBUTE::DWMWA_BORDER_COLOR,
			&BORDER_COLOR, sizeof(BORDER_COLOR));

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		if (GEctx->Dpi_Changed)
		{
			Editor::GameProject::Browser::Update_Dpi_Scale();
			Editor::View::Update_Dpi_Scale();
			GEctx->Dpi_Changed = false;
		}

		Editor::Show_Custom_Caption();
		Editor::View::Main_Dock();

		Editor::View::Show();

		ImGui::ShowDemoWindow();

		if (Editor::GameProject::Project_Opened() is_false)
		{
			Editor::Show_Project_Browser();
			if (Editor::GameProject::Project_Opened()) Editor::GameProject::Save_Project_Open_Datas();
		}

		// Editor::View::Show();

		// Rendering
		// Editor::On_Frame_End();
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

	Editor::Close();
}

void Editor::Close()
{
	WaitForLastSubmittedFrame();

	// Cleanup
	g_p_icon_texture->Release();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow((HWND)GEctx->Hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	delete GEctx;
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

	if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
		g_pd3dCommandList->Close() != S_OK)
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

void Editor::Show_Custom_Caption()
{
	ImGuiViewport*	 viewport		   = ImGui::GetMainViewport();
	ImGuiWindowFlags window_flags	   = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
	GImGui->NextWindowData.Flags	  |= ImGuiNextWindowDataFlags_HasViewport;
	GImGui->NextWindowData.ViewportId  = viewport->ID;
	ImVec2 menuSize					   = viewport->Size;
	menuSize.y						   = CAPTION_HIGHT * GEctx->Dpi_Scale;

	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(menuSize);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, GImGui->Style.Colors[ImGuiCol_TitleBg]);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, menuSize);	   // Lift normal size constraint
	if (ImGui::Begin("MainMenu", NULL, window_flags))
	{
		ImGui::PopStyleVar(4);
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(ImVec2(CAPTION_ICON_CPOS.x * GEctx->Dpi_Scale, CAPTION_ICON_CPOS.y * GEctx->Dpi_Scale));
		ImGui::Image(GEctx->Icon_Texture_Id, ImVec2(CAPTION_ICON_SIZE.x * GEctx->Dpi_Scale, CAPTION_ICON_SIZE.y * GEctx->Dpi_Scale));

		Editor::View::Main_Menu();

		Editor::Draw_Caption_Buttons();
	}

	ImGui::End();
}

void Editor::Draw_Caption_Buttons()
{
	float  dpi_scale				   = GEctx->Dpi_Scale;
	auto   style					   = GImGui->Style;
	float  caption_button_icon_width   = 5.f * dpi_scale;
	float  caption_restore_icon_offset = 2.f * dpi_scale;
	ImVec2 caption_button_size		   = ImVec2(CAPTION_BUTTON_SIZE.x * dpi_scale, CAPTION_BUTTON_SIZE.y * dpi_scale);
	ImVec2 draw_pos					   = ImVec2(GImGui->CurrentWindow->Rect().Max.x - caption_button_size.x * 3, GImGui->CurrentWindow->Pos.y);
	ImVec2 caption_button_icon_center  = ImVec2(draw_pos.x + caption_button_size.x / 2, draw_pos.y + caption_button_size.y / 2);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	if (GEctx->Caption_Hovered_Button != Caption_Button_None)
	{
		ImColor button_color = GEctx->Caption_Held_Button == GEctx->Caption_Hovered_Button ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_ButtonHovered];
		ImVec2	p_min		 = ImVec2(draw_pos.x + CAPTION_BUTTON_SIZE.x * (int)GEctx->Caption_Hovered_Button * GEctx->Dpi_Scale, draw_pos.y);
		ImVec2	p_max		 = ImVec2(p_min.x + CAPTION_BUTTON_SIZE.x * GEctx->Dpi_Scale, draw_pos.y + CAPTION_BUTTON_SIZE.y * GEctx->Dpi_Scale);
		draw_list->AddRectFilled(p_min, p_max, button_color);
	}

	draw_list->AddLine(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y), ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);

	draw_pos.x					 += caption_button_size.x;
	caption_button_icon_center.x += caption_button_size.x;

	if (Is_Window_Maximized(GEctx->Hwnd))
	{
		draw_list->AddRect(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width + caption_restore_icon_offset),
						   ImVec2(caption_button_icon_center.x + caption_button_icon_width - caption_restore_icon_offset, caption_button_icon_center.y + caption_button_icon_width), ImColor(style.Colors[ImGuiCol_Text]), 0.f, ImDrawFlags_None, dpi_scale);

		draw_list->AddLine(ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width + caption_restore_icon_offset),
						   ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);

		draw_list->AddLine(ImVec2(caption_button_icon_center.x - caption_button_icon_width + caption_restore_icon_offset, caption_button_icon_center.y - caption_button_icon_width),
						   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);

		draw_list->AddLine(ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width),
						   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);

		draw_list->AddLine(ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset),
						   ImVec2(caption_button_icon_center.x + caption_button_icon_width - caption_restore_icon_offset, caption_button_icon_center.y + caption_button_icon_width - caption_restore_icon_offset), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);
	}
	else
	{
		draw_list->AddRect(ImVec2(draw_pos.x + caption_button_size.x / 2 - caption_button_icon_width, draw_pos.y + caption_button_size.y / 2 - caption_button_icon_width), ImVec2(draw_pos.x + caption_button_size.x / 2 + caption_button_icon_width, draw_pos.y + caption_button_size.y / 2 + caption_button_icon_width), ImColor(style.Colors[ImGuiCol_Text]), 0.f, ImDrawFlags_None, dpi_scale);
	}

	draw_pos.x					 += caption_button_size.x;
	caption_button_icon_center.x += caption_button_size.x;

	draw_list->AddLine(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width),
					   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);
	draw_list->AddLine(ImVec2(caption_button_icon_center.x - caption_button_icon_width, caption_button_icon_center.y + caption_button_icon_width),
					   ImVec2(caption_button_icon_center.x + caption_button_icon_width, caption_button_icon_center.y - caption_button_icon_width), ImColor(style.Colors[ImGuiCol_Text]), dpi_scale);
}

void Editor::Show_Project_Browser()
{
	if (ImGui::BeginPopupModal("Project_Browser", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
	{
		Editor::GameProject::Browser::Draw();
		ImGui::EndPopup();
	}

	ImGui::OpenPopup("Project_Browser");
}

bool Editor::Is_Window_Maximized(void* hwnd)
{
	WINDOWPLACEMENT placement = { 0 };
	placement.length		  = sizeof(WINDOWPLACEMENT);
	if (::GetWindowPlacement((HWND)hwnd, &placement))
	{
		return placement.showCmd == SW_SHOWMAXIMIZED;
	}

	return false;
}

bool IsMousePosInClient(POINT mouseScreenPos)
{
	return mouseScreenPos.y >= CAPTION_HIGHT * GEctx->Dpi_Scale or GEctx->Main_Menu_Rect.Contains(ImVec2((float)mouseScreenPos.x, (float)mouseScreenPos.y));
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// todo change imgui source file
	if (msg == WM_NCMOUSEMOVE)
	{
		switch (wParam)
		{
		case HTCAPTION:
		{
			GEctx->Caption_Hovered_Button = Caption_Button_None;
			break;
		}
		case HTMINBUTTON:
		{
			GEctx->Caption_Hovered_Button = CaptionButton::Caption_Button_Min;
			break;
		}
		case HTMAXBUTTON:
		{
			GEctx->Caption_Hovered_Button = CaptionButton::Caption_Button_Max;
			break;
		}
		case HTCLOSE:
		{
			GEctx->Caption_Hovered_Button = CaptionButton::Caption_Button_Close;
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

		if (IsMousePosInClient(cursor_point))
		{
			GEctx->Caption_Hovered_Button = Caption_Button_None;
			GEctx->Caption_Held_Button	  = Caption_Button_None;
			return HTCLIENT;
		}

		float cxDiff = ImGui::GetMainViewport()->Size.x - cursor_point.x;
		if (cxDiff > CAPTION_BUTTON_SIZE.x * GEctx->Dpi_Scale * 3)
		{
			return HTCAPTION;
		}
		if (cxDiff > CAPTION_BUTTON_SIZE.x * GEctx->Dpi_Scale * 2)
		{
			return HTMINBUTTON;
		}
		if (cxDiff > CAPTION_BUTTON_SIZE.x * GEctx->Dpi_Scale)
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
		// if (GEctx->Main_Menu_Rect.Contains(ImVec2((float)cursor_point.x, (float)cursor_point.y)) == false)
		//{
		//	GEctx->Main_Menu_State = Menu_State_None;
		// }

		if (GEctx->Caption_Hovered_Button == Caption_Button_None)
		{
			break;
			// return 0;
		}

		GEctx->Caption_Held_Button = GEctx->Caption_Hovered_Button;
		return 0;
	}
	case WM_NCLBUTTONUP:
	{
		if (GEctx->Caption_Held_Button != GEctx->Caption_Hovered_Button)
		{
			break;
		}

		GEctx->Caption_Held_Button = Caption_Button_None;

		switch (GEctx->Caption_Hovered_Button)
		{
		case Caption_Button_Min:
			::ShowWindow(hWnd, SW_MINIMIZE);
			return 0;
		case Caption_Button_Max:
		{
			int mode = Editor::Is_Window_Maximized(GEctx->Hwnd) ? SW_NORMAL : SW_MAXIMIZE;
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

		UINT dpi = GetDpiForWindow(hWnd);

		int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
		int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

		NCCALCSIZE_PARAMS* params				 = (NCCALCSIZE_PARAMS*)lParam;
		RECT*			   requested_client_rect = params->rgrc;

		requested_client_rect->right  -= frame_x + padding;
		requested_client_rect->left	  += frame_x + padding;
		requested_client_rect->bottom -= frame_y + padding;

		if (Editor::Is_Window_Maximized(hWnd))
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
		RECT* rect = (RECT*)lParam;
		IM_ASSERT(LOWORD(wParam) == HIWORD(wParam));
		::SetWindowPos(hWnd, NULL, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER);
		Editor::Update_Dpi_Scale((float)LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI);
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
