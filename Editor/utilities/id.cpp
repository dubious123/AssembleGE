#include "id.h"
#include <format>
#include <sstream>

namespace Editor::ID
{
	namespace
	{
		static std::vector<unsigned long> _tag_vec(DataType_Count);
	}

	AssembleID GetNew(Editor_DataType type, unsigned __int16 data)
	{
		AssembleID res;
		res.Type = type;
		res.Data = data;
		res.Tag	 = ++_tag_vec[type];
		return res;
	}

	std::string AssembleID::ToString()
	{
		auto formatted = std::format("{:04x}-{:04x}-{:08x}", Type, Data, Tag);
		return formatted;
	}

	AssembleID Read(std::string str)
	{
		bool		res	   = true;
		auto		stream = std::istringstream(str);
		std::string str_type, str_data, str_tag;
		std::getline(stream, str_type, '-');
		std::getline(stream, str_data, '-');
		std::getline(stream, str_tag, '-');
		res &= str_type.empty() is_false;
		res &= str_data.empty() is_false;
		res &= str_tag.empty() is_false;
		res &= str_type.length() == sizeof(AssembleID::Type);
		res &= str_data.length() == sizeof(AssembleID::Data);
		res &= str_tag.length() == sizeof(AssembleID::Tag);

		if (res is_false) return AssembleID { DataType_InValid, 0, 0 };

		unsigned __int8	 type = std::stoi(str_type, nullptr, 16);
		unsigned __int16 data = std::stoi(str_data, nullptr, 16);
		unsigned __int32 tag  = std::stoi(str_tag, nullptr, 16);
		return AssembleID { type, data, tag };
	}
}	 // namespace Editor::ID