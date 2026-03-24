#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12) && defined(AGE_PLATFORM_WINDOW)
extern "C"
{
__declspec(dllexport) extern const uint32_t D3D12SDKVersion = 619;
__declspec(dllexport) extern const char*	D3D12SDKPath	= ".\\D3D12\\";
}
#endif

// external
#line 1 "age_external_wrapper_mikktspace.cpp"
#include "age_external_wrapper_mikktspace.cpp"

#line 1 "age_external_wrapper_earcut.cpp"
#include "age_external_wrapper_earcut.cpp"

#line 1 "age_asset.cpp"
#include "age_asset.cpp"

#line 1 "age_asset_mesh_editable.cpp"
#include "age_asset_mesh_editable.cpp"
#line 1 "age_asset_mesh_baked.cpp"
#include "age_asset_mesh_baked.cpp"

#line 1 "age_request.cpp"
#include "age_request.cpp"

#line 1 "age_input.cpp"
#include "age_input.cpp"

#line 1 "age_platform_backend_window.cpp"
#include "age_platform_backend_window.cpp"

#line 1 "age_ui_id.cpp"
#include "age_ui_id.cpp"

#line 1 "age_ui.cpp"
#include "age_ui.cpp"

// graphics
#line 1 "age_graphics_backend_dx12.cpp"
#include "age_graphics_backend_dx12.cpp"

#line 1 "age_graphics_backend_dx12_command.cpp"
#include "age_graphics_backend_dx12_command.cpp"

#line 1 "age_graphics_backend_dx12_util.cpp"
#include "age_graphics_backend_dx12_util.cpp"

#line 1 "age_graphics_backend_dx12_command_signature.cpp"
#include "age_graphics_backend_dx12_command_signature.cpp"

#line 1 "age_graphics_backend_dx12_render_surface.cpp"
#include "age_graphics_backend_dx12_render_surface.cpp"

#line 1 "age_graphics_backend_dx12_resource.cpp"
#include "age_graphics_backend_dx12_resource.cpp"

#line 1 "age_graphics_backend_dx12_shader.cpp"
#include "age_graphics_backend_dx12_shader.cpp"

#line 1 "age_graphics_backend_dx12_rt.cpp"
#include "age_graphics_backend_dx12_rt.cpp"

#line 1 "age_graphics_backend_dx12_render_pipeline_forward_plus_stages.cpp"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus_stages.cpp"

#line 1 "age_graphics_backend_dx12_render_pipeline_forward_plus.cpp"
#include "age_graphics_backend_dx12_render_pipeline_forward_plus.cpp"

#line 1 "age_runtime.cpp"
#include "age_runtime.cpp"