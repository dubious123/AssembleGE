#pragma once
#include "../editor_common.h"
#include "../editor.h"
#include <vector>
#include <string>

namespace Editor::ID
{
	struct AssembleID
	{
		unsigned __int8	 Type;
		unsigned __int16 Data;
		unsigned __int32 Tag;

		std::string ToString();
	};

	AssembleID GetNew(Editor_DataType type, unsigned __int16 data = 0);

	AssembleID Read(std::string str);
}	 // namespace Editor::ID