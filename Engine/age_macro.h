#pragma once
#define is_false == false

#define is_true == true

#define is_nullptr == nullptr

#define is_not_nullptr != nullptr

#define FWD(x) std::forward<decltype(x)>(x)

#define STR_HASH(x) (age::meta::MM<sizeof(x) - 1>::crc32(x))

#ifdef AGE_DEBUG
	#define DEBUG_LOG(fmt, ...)                                               \
		do                                                                    \
		{                                                                     \
			std::cout << std::format(fmt __VA_OPT__(, ) __VA_ARGS__) << '\n'; \
		}                                                                     \
		while (false)
#else
	#define DEBUG_LOG(message)
#endif

#if defined(AGE_DEBUG)
	#define AGE_DEBUG_OP(op) op
#else
	#define AGE_DEBUG_OP(op)
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
	#define AGE_ASSERT(expression)                        \
		do                                                \
		{                                                 \
			if (expression is_false)                      \
			{                                             \
				std::println("  expr : {}", #expression); \
				std::println("  file : {}", __FILE__);    \
				std::println("  line : {}", __LINE__);    \
				AGE_DEBUG_BREAK();                        \
			}                                             \
		}                                                 \
		while (false)
#else
	#define AGE_ASSERT(expression)
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
		#define AGE_HR_CHECK(expression) (expression)
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
		#define AGE_WIN32_CHECK(expression) (expression)
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
	}\
