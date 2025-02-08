#include "pch.h"
#include "editor_common.h"
#include "game.h"
#include "editor_background.h"
#include "editor_ctx_item.h"

namespace
{
	auto _vs_opened = false;
}

namespace
{
#define RETURN_ON_FAIL(expression) \
	result = (expression);         \
	if (FAILED(result))            \
		return false;

	bool _open_visual_studio()
	{
		auto result = HRESULT {};
		auto clsid	= CLSID {};
		auto punk	= CComPtr<IUnknown> {};
		auto dte	= CComPtr<EnvDTE::_DTE> {};

		auto solution_path = CComBSTR { "C:\\Users\\Jonghun\\Desktop\\test1\\test1.sln" };

		RETURN_ON_FAIL(::CLSIDFromProgID(L"VisualStudio.DTE", &clsid));

		auto rot		  = CComPtr<IRunningObjectTable> {};
		auto enum_moniker = CComPtr<IEnumMoniker> {};
		auto dte_moniker  = CComPtr<IMoniker> {};
		auto moniker	  = CComPtr<IMoniker> {};

		RETURN_ON_FAIL(::GetRunningObjectTable(0, &rot));

		RETURN_ON_FAIL(rot->EnumRunning(&enum_moniker));

		RETURN_ON_FAIL(::CreateClassMoniker(clsid, &dte_moniker));

		auto solution = CComPtr<EnvDTE::_Solution> {};
		auto fectched = 0ul;
		while (enum_moniker->Next(1, &moniker, &fectched) == S_OK)
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

			dte = punk;

			if (dte is_nullptr)
			{
				goto CONTINUE;
			}

			RETURN_ON_FAIL(dte->get_Solution(&solution));

			RETURN_ON_FAIL(solution->get_FullName(&full_name));

			if (full_name == solution_path)
			{
				break;
			}

		CONTINUE:
			dte		 = nullptr;
			moniker	 = nullptr;
			punk	 = nullptr;
			solution = nullptr;
		}

		if (dte is_nullptr)
		{
			RETURN_ON_FAIL(::CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, EnvDTE::IID__DTE, (void**)&dte));
			RETURN_ON_FAIL(dte->get_Solution(&solution));
		}

		auto vs_opened = VARIANT_BOOL { FALSE };
		RETURN_ON_FAIL(solution->get_IsOpen(&vs_opened));

		if (vs_opened is_false)
		{
			RETURN_ON_FAIL(solution->Open(solution_path));
		}

		auto vs_window = CComPtr<EnvDTE::Window> {};
		dte->get_MainWindow(&vs_window);
		vs_window->Activate();

		dte->put_UserControl(VARIANT_TRUE);
		RETURN_ON_FAIL(CoDisconnectObject(dte, NULL));

		_vs_opened = true;
		return true;
	}
}	 // namespace

namespace
{
	editor_command _cmd_open_vs {
		"Open Visual Studio",
		ImGuiKey_None,
		[](editor_id _) {
			return _vs_opened is_false;
		},
		[](editor_id _) {
			_vs_opened = true;
			editor::background::add(Background_Visual_Studio, []() { 
			auto res = HRESULT{};
			res = CoInitialize(nullptr);
			assert(SUCCEEDED(res));
			auto _vs_opened = _open_visual_studio();
			assert(_vs_opened and "open visual studio failed"); 
			CoUninitialize(); });
		}
	};
}	 // namespace

namespace editor::game::code
{
	void init()
	{
		// if (FAILED(CoInitialize(NULL)))
		//{
		//	assert(false and "CoInitialize failed");
		//	return;
		// }
	}

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

	bool open_visual_studio()
	{
		_cmd_open_vs();
		return true;
	}

	void deinit()
	{
		/*CoUninitialize();*/
	}

	void on_project_loaded()
	{
		auto res = editor::ctx_item::add_context_item("Main Menu\\Visual Studio\\Open", &_cmd_open_vs);
		assert(res);
	}
}	 // namespace editor::game::code