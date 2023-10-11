#pragma once
#include <string>
#include <type_traits>
#include <vector>
#include "editor_constants.h"
#include "utilities/Xml.h"
#include "logger.h"

#define is_true		   == true
#define is_false	   == false
#define is_not_nullptr != nullptr
#define is_nullptr	   == nullptr

#define STR_SPLIT	  "@@"
#define STR_SPLIT_LEN std::char_traits<char>::length(STR_SPLIT)

template <typename Arg1, typename Arg2>
constexpr typename std::common_type<Arg1, Arg2>::type Variadic_Max(Arg1&& arg1, Arg2&& arg2);

template <typename Arg, typename... Args>
constexpr typename std::common_type<Arg, Args...>::type Variadic_Max(Arg&& arg, Args&&... args);

template <typename Arg1, typename Arg2>
constexpr typename std::common_type<Arg1, Arg2>::type Variadic_Min(Arg1&& arg1, Arg2&& arg2);

template <typename Arg, typename... Args>
constexpr typename std::common_type<Arg, Args...>::type Variadic_Min(Arg&& arg, Args&&... args);

inline consteval static size_t Split(std::string_view source, std::string_view splitter);

template <typename Arg1, typename Arg2>
constexpr typename std::common_type<Arg1, Arg2>::type Variadic_Max(Arg1&& arg1, Arg2&& arg2)
{
	return arg1 > arg2 ? std::forward<Arg1>(arg1) : std::forward<Arg2>(arg2);
}

template <typename Arg, typename... Args>
constexpr typename std::common_type<Arg, Args...>::type Variadic_Max(Arg&& arg, Args&&... args)
{
	return Variadic_Max(std::forward<Arg>(arg), Variadic_Max(std::forward<Args>(args)...));
}

template <typename Arg1, typename Arg2>
constexpr typename std::common_type<Arg1, Arg2>::type Variadic_Min(Arg1&& arg1, Arg2&& arg2)
{
	return arg1 < arg2 ? std::forward<Arg1>(arg1) : std::forward<Arg2>(arg2);
}

template <typename Arg, typename... Args>
constexpr typename std::common_type<Arg, Args...>::type Variadic_Min(Arg&& arg, Args&&... args)
{
	return Variadic_Min(std::forward<Arg>(arg), Variadic_Min(std::forward<Args>(args)...));
}

inline consteval static size_t Split(std::string_view source, std::string_view splitter)
{
	return source.find(splitter);
}
