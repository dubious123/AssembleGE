#include "pch.h"
#include "editor_common.h"
#include "game.h"
#include "editor_background.h"
#include "editor_ctx_item.h"
#include "editor_utilities.h"
#include "editor_models.h"
#include "platform.h"
#import "libid:1a234bc3-6b3a-403c-8161-a4b2f944d75b" raw_interfaces_only, raw_native_types, named_guids

// RETURN_ON_FAIL(::CLSIDFromProgID(L"Microsoft.VisualStudio.VCProjectEngine", &clsid));

// #import "libid:fbbf3c60-2428-11d7-8bf6-00b0d03daa06"
// #import "C:\\Program Files\\Microsoft Visual Studio\\2022\\Preview\\Common7\\IDE\\PublicAssemblies\\Microsoft.VisualStudio.VCProject.dll"

struct __declspec(uuid("CE374261-7F7E-4FB8-AF6B-7C33B8F06D9C")) IVSEnvDTE : public IUnknown
{
	virtual HRESULT __stdcall init(BSTR sln_path)																	   = 0;
	virtual HRESULT __stdcall open_vs()																				   = 0;
	virtual HRESULT __stdcall monitor_vs_opened(void* p_bool)														   = 0;
	virtual HRESULT __stdcall build_if_needed()																		   = 0;
	virtual HRESULT __stdcall up_to_date(void* p_bool)																   = 0;
	virtual HRESULT __stdcall edit(BSTR begin_path, BSTR end_path, BSTR replace, BSTR search_end, VARIANT_BOOL repeat) = 0;
	virtual HRESULT __stdcall deinit()																				   = 0;
};

// constants
namespace
{
#define CONFIG_STR "DebugEditor"
}	 // namespace

namespace
{
	auto _hresult = HRESULT {};

	auto* _dte_wrapper = (IVSEnvDTE*)nullptr;

	// updated by event_loop
	volatile auto _vs_opened = false;

	auto _vs_open_trying = false;
	auto _com_h_edited	 = false;
}	 // namespace

namespace
{
#define RETURN_ON_FAIL(expression) \
	_hresult = (expression);       \
	if (FAILED(_hresult))          \
		return false;

	void _event_loop()
	{
		// vs opened

	Continue:
		Sleep(100);
		editor::background::add(Background_Visual_Studio, _event_loop);
	}

	bool _is_up2date()
	{
		auto res = false;
		_dte_wrapper->up_to_date(&res);
		return res;
	}
}	 // namespace

namespace
{
	editor_command _cmd_open_vs {
		"Open Visual Studio",
		ImGuiKey_None,
		[](editor_id _) {
			return _vs_opened is_false and _vs_open_trying is_false;
		},
		[](editor_id _) {
			_vs_open_trying = true;
			editor::background::add(Background_Visual_Studio, []() {
				//_open_visual_studio();
				_vs_opened		= SUCCEEDED(_dte_wrapper->open_vs());
				_vs_open_trying = false;
				if (FAILED(_hresult))
				{
					editor::logger::info("open visual studio failed with hresult : {}", _hresult);
				}
			});
		}
	};
}	 // namespace

namespace editor::game::code
{
	void init()
	{
		background::add_init(Background_Visual_Studio, []() {
			CLSID clsid;
			auto  res = SUCCEEDED(::CoInitialize(nullptr));
			res		  = SUCCEEDED(::CLSIDFromString(L"{6BA04EBF-3841-4A5E-AF68-8DFD4E038A00}", &clsid));
			res		  = SUCCEEDED(::CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, __uuidof(IVSEnvDTE), (void**)&_dte_wrapper));
			res		  = SUCCEEDED(_dte_wrapper->monitor_vs_opened((void*)&_vs_opened));
			return res;
		});
		background::add_deinit(Background_Visual_Studio, []() { ::CoUninitialize(); return true; });

		platform::add_on_wm_activate([]() {
			if (_vs_opened and _is_up2date() is_false)
			{
				game::unload_dll();
				background::add(Background_Visual_Studio, []() { _dte_wrapper->build_if_needed(); }, game::load_dll);
			}
		});
		// background::add(Background_Visual_Studio, _event_loop);
		//  return 0;
	}

	bool open_visual_studio()
	{
		_cmd_open_vs();
		return true;
	}

	void deinit()
	{
	}

	void on_project_loaded()
	{
		background::add(Background_Visual_Studio, []() {
			logger::info("dte init {}", SUCCEEDED(_dte_wrapper->init(CComBSTR(editor::game::get_pproject()->sln_path.c_str()))));
		});
		auto res = editor::ctx_item::add_context_item("Main Menu\\Visual Studio\\Open", &_cmd_open_vs);
		assert(res);
	}

	void on_project_unloaded()
	{
		background::add(Background_Visual_Studio, []() {
			logger::info("dte deinit {}", SUCCEEDED(_dte_wrapper->init(CComBSTR(editor::game::get_pproject()->sln_path.c_str()))));
		});
	}

	void edit(const std::string begin_path, const std::string end_path, const std::string replace, bool repeat, const std::string search_end)
	{
		if (editor::game::get_pproject()->is_opened is_false)
		{
			return;
		}

		editor::background::add(Background_Visual_Studio, [=]() {
			if (SUCCEEDED(_dte_wrapper->edit(
					CComBSTR(begin_path.c_str()),
					CComBSTR(end_path.c_str()),
					CComBSTR(replace.c_str()),
					CComBSTR(search_end.c_str()),
					repeat ? VARIANT_TRUE : VARIANT_FALSE)) is_false)
			{
				editor::logger::info("code edit failed");
			}
		});
	}
}	 // namespace editor::game::code

bool visual_studio_open_file(const char* filename, unsigned int line)
{
	HRESULT result;
	CLSID	clsid;
	result = ::CLSIDFromProgID(L"VisualStudio.DTE", &clsid);
	if (FAILED(result))
		return false;

	CComPtr<IUnknown> punk;
	result = ::GetActiveObject(clsid, NULL, &punk);
	if (FAILED(result))
		return false;

	CComPtr<EnvDTE::_DTE> DTE;
	DTE = punk;

	CComPtr<EnvDTE::ItemOperations> item_ops;
	result = DTE->get_ItemOperations(&item_ops);
	if (FAILED(result))
		return false;

	CComBSTR				bstrFileName(filename);
	CComBSTR				bstrKind(EnvDTE::vsViewKindTextView);
	CComPtr<EnvDTE::Window> window;
	result = item_ops->OpenFile(bstrFileName, bstrKind, &window);
	if (FAILED(result))
		return false;

	CComPtr<EnvDTE::Document> doc;
	result = DTE->get_ActiveDocument(&doc);
	if (FAILED(result))
		return false;

	CComPtr<IDispatch> selection_dispatch;
	result = doc->get_Selection(&selection_dispatch);
	if (FAILED(result))
		return false;

	CComPtr<EnvDTE::TextSelection> selection;
	result = selection_dispatch->QueryInterface(&selection);
	if (FAILED(result))
		return false;

	result = selection->GotoLine(line, TRUE);
	if (FAILED(result))
		return false;

	return true;
}
