#pragma once
#define is_false == false

#define is_true == true

#define is_nullptr == nullptr

#define is_not_nullptr != nullptr

#define FWD(x) std::forward<decltype(x)>(x)

#define STR_HASH(x) (age::meta::MM<sizeof(x) - 1>::crc32(x))

#ifdef _DEBUG
	#define DEBUG_LOG(fmt, ...)                                               \
		do                                                                    \
		{                                                                     \
			std::cout << std::format(fmt __VA_OPT__(, ) __VA_ARGS__) << '\n'; \
		}                                                                     \
		while (0)
#else
	#define DEBUG_LOG(message)
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