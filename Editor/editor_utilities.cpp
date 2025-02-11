#include "pch.h"
#include "editor_utilities.h"

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

std::vector<std::string> editor::utilities::split_string(const std::string& str, const std::string& delims)
{
	auto re	   = std::regex(std::format("[{}]", delims));
	auto first = std::sregex_token_iterator { str.begin(), str.end(), re, -1 };
	auto last  = std::sregex_token_iterator {};
	return { first, last };
}

std::wstring editor::utilities::str_to_wstr(std::string str)
{
	return std::wstring(str.c_str(), str.c_str() + strlen(str.c_str()));
}