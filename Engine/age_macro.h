#pragma once
#define is_false == false

#define is_true == true

#define is_nullptr == nullptr

#define is_not_nullptr != nullptr

#define FWD(x) std::forward<decltype(x)>(x)

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
	#define AGE_MSVC_WARNING_PUSH		__pragma(warning(push))
	#define AGE_MSVC_WARNING_POP		__pragma(warning(pop))
	#define AGE_MSVC_WARNING_DISABLE(n) __pragma(warning(disable : n))
#else
	#define AGE_MSVC_WARNING_PUSH
	#define AGE_MSVC_WARNING_POP
	#define AGE_MSVC_WARNING_DISABLE(n)
#endif

#define AGE_FUNC(overloaded_func_name) [] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept -> decltype(auto) INLINE_LAMBDA_BACK { \
	if constexpr (requires { overloaded_func_name(FWD(arg)...); })                                                           \
	{                                                                                                                        \
		return overloaded_func_name(FWD(arg)...);                                                                            \
	}                                                                                                                        \
	else                                                                                                                     \
	{                                                                                                                        \
		return age::ecs::system::detail::invalid_sys_call{};                                                                 \
	}                                                                                                                        \
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


#define AGE_ENUM_FLAG_OPERATORS(T)                                                             \
	[[nodiscard]] FORCE_INLINE constexpr T                                                     \
	operator|(T a, T b) noexcept                                                               \
	{                                                                                          \
		return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));                  \
	}                                                                                          \
	FORCE_INLINE T&                                                                            \
	operator|=(T& a, T b) noexcept                                                             \
	{                                                                                          \
		a = a | b;                                                                             \
		return a;                                                                              \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE constexpr T                                                     \
	operator&(T a, T b) noexcept                                                               \
	{                                                                                          \
		return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));                  \
	}                                                                                          \
	FORCE_INLINE T&                                                                            \
	operator&=(T& a, T b) noexcept                                                             \
	{                                                                                          \
		a = a & b;                                                                             \
		return a;                                                                              \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE constexpr T                                                     \
	operator~(T a) noexcept                                                                    \
	{                                                                                          \
		return static_cast<T>(~std::to_underlying(a));                                         \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE constexpr T                                                     \
	operator^(T a, T b) noexcept                                                               \
	{                                                                                          \
		return static_cast<T>(std::to_underlying(a) ^ std::to_underlying(b));                  \
	}                                                                                          \
	FORCE_INLINE T&                                                                            \
	operator^=(T& a, T b) noexcept                                                             \
	{                                                                                          \
		a = a ^ b;                                                                             \
		return a;                                                                              \
	}                                                                                          \
	[[nodiscard]] FORCE_INLINE constexpr bool                                                  \
	has_any(T v, T mask) noexcept                                                              \
	{                                                                                          \
		return (std::to_underlying(v) & std::to_underlying(mask)) != 0;                        \
	}                                                                                          \
                                                                                               \
	[[nodiscard]] FORCE_INLINE constexpr bool                                                  \
	has_all(T v, T mask) noexcept                                                              \
	{                                                                                          \
		return (std::to_underlying(v) & std::to_underlying(mask)) == std::to_underlying(mask); \
	}

#define AGE_ENUM_TO_STRING_CASE(enum_name) \
	case __t_enum__::enum_name:            \
	{                                      \
		return #enum_name;                 \
	}

#define AGE_DEFINE_ENUM(enum_class_name, underlying_type, ...)                  \
	enum class enum_class_name : underlying_type                                \
	{                                                                           \
		__VA_ARGS__                                                             \
	};                                                                          \
	constexpr inline std::string_view to_string(enum_class_name e)              \
	{                                                                           \
		using __t_enum__ = enum_class_name;                                     \
		switch (e)                                                              \
		{                                                                       \
			FOR_EACH(AGE_ENUM_TO_STRING_CASE, __VA_ARGS__)                      \
		default:                                                                \
		{                                                                       \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
		}                                                                       \
		}                                                                       \
	}