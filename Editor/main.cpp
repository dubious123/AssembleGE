// Dear ImGui: standalone example application for DirectX 12
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important: to compile on 32-bit systems, the DirectX12 backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include "pch.h"
#include "editor.h"

// #pragma comment(lib, "external/lib/spdlog.lib")
// #pragma comment(lib, "external/lib/nfd.lib")
#pragma comment(lib, "nfd.lib")
#pragma comment(lib, "Rpcrt4.lib")	  // for uuid
#pragma comment(lib, "spdlog.lib")

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	editor::init();
	editor::run();
	return 0;
}