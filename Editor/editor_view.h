#include "Editor.h"
#include "editor_common.h"

namespace Editor::View
{
#define max(a, b) a > b ? a : b;
	void Update_Dpi_Scale();
	void Init();
	bool Add_Callback_To_Main_Menu(std::string path, void (*callback)());
	void Main_Menu();
	void Main_Dock();

	void Show();
}	 // namespace Editor::View