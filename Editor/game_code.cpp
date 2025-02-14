#include "pch.h"
#include "editor_common.h"
#include "game.h"
#include "editor_background.h"
#include "editor_ctx_item.h"
#include "editor_utilities.h"
#import "libid:1a234bc3-6b3a-403c-8161-a4b2f944d75b" raw_interfaces_only, raw_native_types, named_guids

// RETURN_ON_FAIL(::CLSIDFromProgID(L"Microsoft.VisualStudio.VCProjectEngine", &clsid));

// #import "libid:fbbf3c60-2428-11d7-8bf6-00b0d03daa06"
// #import "C:\\Program Files\\Microsoft Visual Studio\\2022\\Preview\\Common7\\IDE\\PublicAssemblies\\Microsoft.VisualStudio.VCProject.dll"

struct __declspec(uuid("CE374261-7F7E-4FB8-AF6B-7C33B8F06D9C")) IVSEnvDTE : public IUnknown
{
	virtual HRESULT __stdcall open_vs(BSTR sln_path)		  = 0;
	virtual HRESULT __stdcall monitor_vs_opened(void* p_bool) = 0;
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

	auto _dte				 = CComPtr<EnvDTE::_DTE> {};
	auto _solution			 = CComPtr<EnvDTE::_Solution> {};
	auto _project			 = CComPtr<EnvDTE::Project> {};
	auto _vs_main_window	 = CComPtr<EnvDTE::Window> {};
	auto _vs_com_h_proj_item = CComPtr<EnvDTE::ProjectItem> {};

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

	bool _open_visual_studio()
	{
		if (_vs_opened)
		{
			return true;
		}

		_dte			= nullptr;
		_solution		= nullptr;
		_project		= nullptr;
		_vs_main_window = nullptr;

		auto result		   = HRESULT {};
		auto clsid		   = CLSID {};
		auto punk		   = CComPtr<IUnknown> {};
		auto solution_path = CComBSTR { editor::game::get_pproject()->sln_path.c_str() };

		auto rot		  = CComPtr<IRunningObjectTable> {};
		auto enum_moniker = CComPtr<IEnumMoniker> {};
		auto dte_moniker  = CComPtr<IMoniker> {};
		auto moniker	  = CComPtr<IMoniker> {};

		RETURN_ON_FAIL(::CLSIDFromProgID(L"VisualStudio.DTE", &clsid));

		RETURN_ON_FAIL(::GetRunningObjectTable(0, &rot));

		RETURN_ON_FAIL(rot->EnumRunning(&enum_moniker));

		RETURN_ON_FAIL(::CreateClassMoniker(clsid, &dte_moniker));

		while (enum_moniker->Next(1, &moniker, /*&fectched*/ nullptr) == S_OK)
		{
			auto full_name = CComBSTR {};

			if (FAILED(moniker->IsEqual(dte_moniker)))
			{
				goto CONTINUE;
			}

			if (FAILED(rot->GetObject(moniker, &punk)))
			{
				goto CONTINUE;
			}

			_dte = punk;

			if (_dte is_nullptr)
			{
				goto CONTINUE;
			}

			RETURN_ON_FAIL(_dte->get_Solution(&_solution));

			RETURN_ON_FAIL(_solution->get_FullName(&full_name));

			if (full_name == solution_path)
			{
				break;
			}

		CONTINUE:
			_dte	  = nullptr;
			_solution = nullptr;
			moniker	  = nullptr;
			punk	  = nullptr;
		}

		if (_dte is_nullptr)
		{
			RETURN_ON_FAIL(::CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, EnvDTE::IID__DTE, (void**)&_dte));
			RETURN_ON_FAIL(_dte->get_Solution(&_solution));
		}

		auto vs_opened = VARIANT_BOOL { FALSE };
		RETURN_ON_FAIL(_solution->get_IsOpen(&vs_opened));

		if (vs_opened is_false)
		{
			RETURN_ON_FAIL(_solution->Open(solution_path));
		}

		{
			auto projects	= CComPtr<EnvDTE::Projects> {};
			auto proj_count = 0l;
			RETURN_ON_FAIL(_solution->get_Projects(&projects));
			RETURN_ON_FAIL(_dte->get_MainWindow(&_vs_main_window));
			RETURN_ON_FAIL(_vs_main_window->Activate());
			RETURN_ON_FAIL(projects->get_Count(&proj_count));

			auto w_proj_name = editor::utilities::str_to_wstr(editor::game::get_pproject()->name);
			for (auto idx : std::views::iota(1l) | std::views::take(proj_count))
			{
				auto proj	   = CComPtr<EnvDTE::Project> {};
				auto var_idx   = VARIANT {};
				auto proj_name = BSTR {};
				var_idx.vt	   = VT_INT;
				var_idx.lVal   = idx;
				RETURN_ON_FAIL(projects->Item(var_idx, &proj));
				RETURN_ON_FAIL(proj->get_Name(&proj_name));
				if (proj_name == w_proj_name)
				{
					_project = proj;
					break;
				}
			}

			if (_project is_nullptr)
			{
				return false;
			}
		}

		{
			auto items		= CComPtr<EnvDTE::ProjectItems> {};
			auto item_count = 0l;
			RETURN_ON_FAIL(_project->get_ProjectItems(&items));
			RETURN_ON_FAIL(items->get_Count(&item_count));

			for (auto idx : std::views::iota(1l) | std::views::take(item_count))
			{
				auto item	   = CComPtr<EnvDTE::ProjectItem> {};
				auto var_idx   = VARIANT {};
				auto item_name = BSTR {};
				var_idx.vt	   = VT_INT;
				var_idx.lVal   = idx;
				RETURN_ON_FAIL(items->Item(var_idx, &item));
				RETURN_ON_FAIL(item->get_Name(&item_name));
				if (item_name == std::wstring(L"components.h"))
				{
					_vs_com_h_proj_item = item;
					break;
				}
			}

			if (_vs_com_h_proj_item is_nullptr)
			{
				return false;
			}
		}

		// let vs stays opened when editor close
		RETURN_ON_FAIL(_dte->put_UserControl(VARIANT_TRUE));
		RETURN_ON_FAIL(::CoDisconnectObject(_dte, NULL));

		// ok to fail
		_vs_main_window->SetFocus();
		return true;
	}

	bool _is_vs_opened()
	{
		auto visible = VARIANT_BOOL { FALSE };
		// auto w_state = EnvDTE::vsWindowState {};
		auto succeed = true;

		if (_vs_main_window)
		{
			// succeed &= SUCCEEDED(_vs_main_window->get_WindowState(&w_state));
			succeed &= SUCCEEDED(_vs_main_window->get_Visible(&visible));
		}

		return succeed and visible;	   // and w_state != EnvDTE::vsWindowStateMinimize;
	}

	bool _is_component_header_edited()
	{
		auto saved = VARIANT_BOOL { FALSE };
		if (_vs_com_h_proj_item)
		{
			_vs_com_h_proj_item->get_Saved(&saved);
		}

		return saved is_false;
	}

	bool _need_build()
	{
	}

	void _clean_up()
	{
		_dte				= nullptr;
		_solution			= nullptr;
		_project			= nullptr;
		_vs_main_window		= nullptr;
		_vs_com_h_proj_item = nullptr;
	}

	void _event_loop()
	{
		// vs opened
		{
			auto prev_vs_opened = _vs_opened;
			_vs_opened			= _is_vs_opened();
			if (prev_vs_opened is_false and _vs_opened is_true)
			{
				editor::logger::info("vs opened");
			}
			else if (prev_vs_opened is_true and _vs_opened is_false)
			{
				editor::logger::info("vs closed");
				_clean_up();
				goto Continue;
			}
		}

		// component.h edited
		{
			auto prev_com_h_edited = _com_h_edited;
			_com_h_edited		   = _is_component_header_edited();
			if (prev_com_h_edited is_false and _com_h_edited is_true)
			{
				editor::logger::info("need rebuild");
			}
			else if (prev_com_h_edited is_true and _com_h_edited is_false)
			{
				editor::logger::info("saved?");
			}
		}

	Continue:
		Sleep(100);
		editor::background::add(Background_Visual_Studio, _event_loop);
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
				_vs_opened		= SUCCEEDED(_dte_wrapper->open_vs(CComBSTR(editor::game::get_pproject()->sln_path.c_str())));
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
		if (FAILED(CoInitialize(NULL)))
		{
			assert(false and "CoInitialize failed");
			return;
		}

		background::add_init(Background_Visual_Studio, []() {
			CLSID clsid;
			auto  res = SUCCEEDED(::CoInitialize(nullptr));
			res		  = SUCCEEDED(::CLSIDFromString(L"{6BA04EBF-3841-4A5E-AF68-8DFD4E038A00}", &clsid));
			res		  = SUCCEEDED(::CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, __uuidof(IVSEnvDTE), (void**)&_dte_wrapper));
			res		  = SUCCEEDED(_dte_wrapper->monitor_vs_opened((void*)&_vs_opened));
			return res;
		});
		background::add_deinit(Background_Visual_Studio, []() { ::CoUninitialize(); return true; });
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
		auto res = editor::ctx_item::add_context_item("Main Menu\\Visual Studio\\Open", &_cmd_open_vs);
		assert(res);
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
