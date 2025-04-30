#pragma once
#include "__math.h"
#include "__data_structures.h"
#define EDITOR_API extern "C" __declspec(dllexport)

#define is_false == false

#define is_true == true

#define is_nullptr == nullptr

#define is_not_nullptr != nullptr

#ifdef _MSC_VER
	#define no_unique_addr [[msvc::no_unique_address]]
#else
	#define no_unique_addr [[no_unique_address]]
#endif

constexpr std::size_t operator"" _KiB(std::size_t k)
{
	return k * 1024;
}