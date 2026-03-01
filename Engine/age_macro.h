#pragma once

#define c_auto const auto

#define is_false == false

#define is_true == true

#define is_nullptr == nullptr

#define is_not_nullptr != nullptr

#define FWD(x) std::forward<decltype(x)>((x))

// reference will be removed in most cases anyway so perfect forwarding is not necessary
#define BARE_OF(expr) std::remove_cvref_t<decltype((expr))>

#define STR_HASH(x) (age::meta::MM<sizeof(x) - 1>::crc32(x))

#define AGE_DO_WHILE(...)       \
	do                          \
	{                           \
		__VA_OPT__(__VA_ARGS__) \
	}                           \
	while (false)

#ifdef AGE_DEBUG
	#define AGE_DEBUG_LOG(...) AGE_DO_WHILE(__VA_OPT__(std::cout << std::format(__VA_ARGS__) << '\n');)
#else
	#define AGE_DEBUG_LOG(...) AGE_DO_WHILE()
#endif

#if defined(AGE_DEBUG)
	#define AGE_DEBUG_ONLY(expression) expression
#else
	#define AGE_DEBUG_ONLY(expression)
#endif

#if defined(AGE_COMPILER_MSVC)
	#define AGE_WARNING_PUSH			__pragma(warning(push))
	#define AGE_WARNING_POP				__pragma(warning(pop))
	#define AGE_WARNING_DISABLE_MSVC(n) __pragma(warning(disable : n))
	#define AGE_WARNING_DISABLE_GCC(w)
	#define AGE_WARNING_DISABLE_CLANG(w)
#elif defined(AGE_COMPILER_CLANG)
	#define AGE_WARNING_PUSH _Pragma("clang diagnostic push")
	#define AGE_WARNING_POP	 _Pragma("clang diagnostic pop")
	#define AGE_WARNING_DISABLE_MSVC(n)
	#define AGE_WARNING_DISABLE_GCC(w)
	#define AGE_WARNING_DISABLE_CLANG(w) _Pragma(w)
#elif defined(AGE_COMPILER_GCC)
	#define AGE_WARNING_PUSH _Pragma("GCC diagnostic push")
	#define AGE_WARNING_POP	 _Pragma("GCC diagnostic pop")
	#define AGE_WARNING_DISABLE_MSVC(n)
	#define AGE_WARNING_DISABLE_GCC(w) _Pragma(w)
	#define AGE_WARNING_DISABLE_CLANG(w)
#else
#endif

#define AGE_LAMBDA(arg_tpl, ...) [&] INLINE_LAMBDA_FRONT arg_tpl noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	__VA_ARGS__                                                                                                  \
}

#define AGE_FUNC(...) [] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	return __VA_ARGS__(FWD(arg)...);                                                                        \
}

#define AGE_CAPTURE_FUNC(...) [&] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	return __VA_ARGS__(FWD(arg)...);                                                                                 \
}

#define AGE_FUNC_MEM(overloaded_func_name) [this] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	requires requires { overloaded_func_name(FWD(arg)...); }                                                                         \
	return overloaded_func_name(FWD(arg)...);                                                                                        \
}

#define AGE_PROP(property_name)                           \
	FORCE_INLINE constexpr decltype((data.property_name)) \
	property_name() noexcept                              \
	{                                                     \
		return data.property_name;                        \
	}

#if defined(AGE_DEBUG)
	#define AGE_ASSERT(expression, ...)                                            \
		do                                                                         \
		{                                                                          \
			if ((expression)is_false)                                              \
			{                                                                      \
				std::println("  expr : {}", #expression);                          \
				std::println("  file : {}", __FILE__);                             \
				std::println("  line : {}", __LINE__);                             \
				__VA_OPT__(std::println("   msg : {}", std::format(__VA_ARGS__));) \
				AGE_DEBUG_BREAK();                                                 \
			}                                                                      \
		}                                                                          \
		while (false)
#else
	#define AGE_ASSERT(expression, ...)
#endif

#if defined(AGE_DEBUG)
	#define AGE_CHECK(expression, ...)                                             \
		do                                                                         \
		{                                                                          \
			if ((expression)is_false)                                              \
			{                                                                      \
				std::println("  expr : {}", #expression);                          \
				std::println("  file : {}", __FILE__);                             \
				std::println("  line : {}", __LINE__);                             \
				__VA_OPT__(std::println("   msg : {}", std::format(__VA_ARGS__));) \
				AGE_DEBUG_BREAK();                                                 \
			}                                                                      \
		}                                                                          \
		while (false)
#else
	#define AGE_CHECK(expression, ...) AGE_DO_WHILE((expression);)
#endif

#if defined(AGE_COMPILER_MSVC)
	#define AGE_DEBUG_BREAK() __debugbreak()
#elif defined(AGE_COMPILER_CLANG) || defined(AGE_COMPILER_GCC)
	#if __has_builtin(__builtin_debugtrap)
		#define AGE_DEBUG_BREAK() __builtin_debugtrap()
	#else
		#define AGE_DEBUG_BREAK() __builtin_trap()
	#endif
#else
	#error C++ compiler required
#endif

#if defined(AGE_DEBUG)
	#define AGE_UNREACHABLE(...) AGE_DO_WHILE(AGE_DEBUG_LOG(__VA_ARGS__); AGE_DEBUG_BREAK(); std::unreachable();)
#else
	#define AGE_UNREACHABLE(...) AGE_DO_WHILE(std::unreachable();)
#endif

#if defined(AGE_PLATFORM_WINDOW)
	#if defined(AGE_DEBUG)
		#define AGE_HR_CHECK(expression)                                                             \
			do                                                                                       \
			{                                                                                        \
				static_assert(std::is_same_v<std::remove_cvref_t<decltype((expression))>, HRESULT>); \
				HRESULT __hr__ = (expression);                                                       \
				if (FAILED(__hr__))                                                                  \
				{                                                                                    \
					std::println("HRESULT failed: {:x}", (unsigned)__hr__);                          \
					std::println("  expr : {}", #expression);                                        \
					std::println("  file : {}", __FILE__);                                           \
					std::println("  line : {}", __LINE__);                                           \
					AGE_DEBUG_BREAK();                                                               \
				}                                                                                    \
			}                                                                                        \
			while (false)
	#else
		#define AGE_HR_CHECK(expression) AGE_DO_WHILE((expression);)
	#endif

	#if defined(AGE_DEBUG)
		#define AGE_WIN32_CHECK(expression)                               \
			do                                                            \
			{                                                             \
				if ((expression) == 0) [[unlikely]]                       \
				{                                                         \
					const auto __last_err__ = ::GetLastError();           \
					std::println("WIN32 API FAILED: {:x}", __last_err__); \
					std::println("  expr : {}", #expression);             \
					std::println("  file : {}", __FILE__);                \
					std::println("  line : {}", __LINE__);                \
					AGE_DEBUG_BREAK();                                    \
				}                                                         \
			}                                                             \
			while (false)
	#else
		#define AGE_WIN32_CHECK(expression) AGE_DO_WHILE((expression);)
	#endif
#endif

#define AGE_DISABLE_COPY(T)          \
	explicit T(const T&)   = delete; \
	T& operator=(const T&) = delete;

#define AGE_DISABLE_MOVE(T)     \
	explicit T(T&&)	  = delete; \
	T& operator=(T&&) = delete;

#define AGE_DISABLE_COPY_MOVE(T) \
	AGE_DISABLE_COPY(T)          \
	AGE_DISABLE_MOVE(T)

//---[ enum ]------------------------------------------------------------

#define __AGE_ENUM_FLAG_OPERATORS_IMPL__(T, enable_static_member)                              \
	[[nodiscard]] FORCE_INLINE enable_static_member constexpr T                                \
	operator|(T a, T b) noexcept                                                               \
	{                                                                                          \
		return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));                  \
	}                                                                                          \
	FORCE_INLINE enable_static_member T&                                                       \
	operator|=(T& a, T b) noexcept                                                             \
	{                                                                                          \
		a = operator|(a, b);                                                                   \
		return a;                                                                              \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE enable_static_member constexpr T                                \
	operator&(T a, T b) noexcept                                                               \
	{                                                                                          \
		return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));                  \
	}                                                                                          \
	FORCE_INLINE enable_static_member T&                                                       \
	operator&=(T& a, T b) noexcept                                                             \
	{                                                                                          \
		a = operator&(a, b);                                                                   \
		return a;                                                                              \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE enable_static_member constexpr T                                \
	operator~(T a) noexcept                                                                    \
	{                                                                                          \
		return static_cast<T>(~std::to_underlying(a));                                         \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE enable_static_member constexpr T                                \
	operator^(T a, T b) noexcept                                                               \
	{                                                                                          \
		return static_cast<T>(std::to_underlying(a) ^ std::to_underlying(b));                  \
	}                                                                                          \
	FORCE_INLINE enable_static_member T&                                                       \
	operator^=(T& a, T b) noexcept                                                             \
	{                                                                                          \
		a = operator^(a, b);                                                                   \
		return a;                                                                              \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE enable_static_member constexpr bool                             \
	has_any(T v, T mask) noexcept                                                              \
	{                                                                                          \
		return (std::to_underlying(v) & std::to_underlying(mask)) != 0;                        \
	}                                                                                          \
                                                                                               \
	[[nodiscard]] FORCE_INLINE enable_static_member constexpr bool                             \
	has_all(T v, T mask) noexcept                                                              \
	{                                                                                          \
		return (std::to_underlying(v) & std::to_underlying(mask)) == std::to_underlying(mask); \
	}

#define AGE_ENUM_FLAG_OPERATORS(T)		  __AGE_ENUM_FLAG_OPERATORS_IMPL__(T, )
#define AGE_ENUM_FLAG_OPERATORS_MEMBER(T) __AGE_ENUM_FLAG_OPERATORS_IMPL__(T, static)

#define AGE_ENUM_TO_STRING_CASE(enum_name) \
	case __t_enum__::enum_name:            \
	{                                      \
		return #enum_name;                 \
	}

#define AGE_ENUM_TO_WSTRING_CASE(enum_name) \
	case __t_enum__::enum_name:             \
	{                                       \
		return L## #enum_name;              \
	}

#define AGE_DEFINE_ENUM(enum_class_name, underlying_type, ...)                  \
	enum class enum_class_name : underlying_type                                \
	{                                                                           \
		__VA_ARGS__                                                             \
	};                                                                          \
	constexpr inline std::string_view to_string(enum_class_name e) noexcept     \
	{                                                                           \
		using __t_enum__ = enum_class_name;                                     \
		switch (e)                                                              \
		{                                                                       \
			FOR_EACH (AGE_ENUM_TO_STRING_CASE, __VA_ARGS__)                     \
				;                                                               \
		default:                                                                \
		{                                                                       \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
		}                                                                       \
		}                                                                       \
	}                                                                           \
	constexpr inline std::wstring_view to_wstring(enum_class_name e) noexcept   \
	{                                                                           \
		using __t_enum__ = enum_class_name;                                     \
		switch (e)                                                              \
		{                                                                       \
			FOR_EACH (AGE_ENUM_TO_WSTRING_CASE, __VA_ARGS__)                    \
				;                                                               \
		default:                                                                \
		{                                                                       \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
		}                                                                       \
		}                                                                       \
	}                                                                           \
	template <typename t>                                                       \
	requires std::is_same_v<t, enum_class_name>                                 \
	consteval std::size_t size() noexcept                                       \
	{ return age::util::str_to_uint64(AGE_PP_STRINGIFY(AGE_PP_VA_COUNT(__VA_ARGS__))); }


#define AGE_ENUM_DECL_VAL(tpl) AGE_PP_TUPLE_GET_0_I(tpl) = AGE_PP_TUPLE_GET_1_I(tpl)

#define AGE_ENUM_TO_STRING_CASE_VAL(tpl)			AGE_ENUM_TO_STRING_CASE_VAL_IMPL tpl
#define AGE_ENUM_TO_STRING_CASE_VAL_IMPL(name, val) AGE_ENUM_TO_STRING_CASE(name)

#define AGE_ENUM_TO_WSTRING_CASE_VAL(tpl)			 AGE_ENUM_TO_WSTRING_CASE_VAL_IMPL tpl
#define AGE_ENUM_TO_WSTRING_CASE_VAL_IMPL(name, val) AGE_ENUM_TO_WSTRING_CASE(name)

#define AGE_DEFINE_ENUM_WITH_VALUE(enum_class_name, underlying_type, ...)           \
	enum class enum_class_name : underlying_type                                    \
	{                                                                               \
		FOR_EACH_SEP(AGE_ENUM_DECL_VAL, AGE_PP_COMMA_I, __VA_ARGS__)                \
	};                                                                              \
	constexpr inline std::string_view to_string(enum_class_name e) noexcept         \
	{                                                                               \
		using __t_enum__ = enum_class_name;                                         \
		switch (e)                                                                  \
		{                                                                           \
			FOR_EACH (AGE_ENUM_TO_STRING_CASE_VAL, __VA_ARGS__)                     \
			default:                                                                \
			{                                                                       \
				AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
			}                                                                       \
		}                                                                           \
	}                                                                               \
	constexpr inline std::wstring_view to_wstring(enum_class_name e) noexcept       \
	{                                                                               \
		using __t_enum__ = enum_class_name;                                         \
		switch (e)                                                                  \
		{                                                                           \
			FOR_EACH (AGE_ENUM_TO_WSTRING_CASE_VAL, __VA_ARGS__)                    \
			default:                                                                \
			{                                                                       \
				AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
			}                                                                       \
		}                                                                           \
	}                                                                               \
	template <typename t>                                                           \
	requires std::is_same_v<t, enum_class_name>                                     \
	consteval std::size_t size() noexcept                                           \
	{ return age::util::str_to_uint64(AGE_PP_STRINGIFY(AGE_PP_VA_COUNT(__VA_ARGS__))); }


#define AGE_DEFINE_ENUM_MEMBER(enum_class_name, underlying_type, ...)           \
	enum class enum_class_name : underlying_type                                \
	{                                                                           \
		__VA_ARGS__                                                             \
	};                                                                          \
	static constexpr std::string_view to_string(enum_class_name e) noexcept     \
	{                                                                           \
		using __t_enum__ = enum_class_name;                                     \
		switch (e)                                                              \
		{                                                                       \
			FOR_EACH (AGE_ENUM_TO_STRING_CASE, __VA_ARGS__)                     \
				;                                                               \
		default:                                                                \
		{                                                                       \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
		}                                                                       \
		}                                                                       \
	}

//---[ age_request.hpp ]------------------------------------------------------------
#define __AGE_REQUEST_COMBINE_FLAGS__(x) age::subsystem::flags::x |

#define AGE_REQUEST_PHASE(...) \
	phase_meta<FOR_EACH (__AGE_REQUEST_COMBINE_FLAGS__, __VA_ARGS__) age::subsystem::flags{ 0 }>

#define AGE_DEFINE_REQUEST_BEGIN           \
	template <age::request::type req_type> \
	constexpr auto get_req_meta()          \
	{                                      \
		if constexpr (false)               \
		{                                  \
		}

#define AGE_DEFINE_REQUEST(req_type_name, param_type, ...)                                 \
	else if constexpr (req_type == age::request::type::req_type_name)                      \
	{                                                                                      \
		return request_meta<age::request::type::req_type_name, param_type, __VA_ARGS__>{}; \
	}

#define AGE_DEFINE_REQUEST_END                        \
	else                                              \
	{                                                 \
		static_assert(false, "invalid request type"); \
	}                                                 \
	}
//---------------------------------------------------------------------------------------