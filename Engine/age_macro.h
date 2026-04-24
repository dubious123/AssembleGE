#pragma once

#define c_auto const auto

#define is_false == false

#define is_true == true

#define is_nullptr == nullptr

#define is_not_nullptr != nullptr

#define FWD(x) std::forward<decltype(x)>((x))

// reference will be removed in most cases anyway so perfect forwarding is not necessary
#define BARE_OF(expr) std::remove_cvref_t<decltype((expr))>

#define IS_CONST(expr) std::is_const_v<std::remove_reference_t<decltype((expr))>>

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

#define AGE_FUNC(...) [&] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	return __VA_ARGS__(FWD(arg)...);                                                                         \
}

#define AGE_CAPTURE_FUNC(...) [&] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	return __VA_ARGS__(FWD(arg)...);                                                                                 \
}

#define AGE_FUNC_MEM(overloaded_func_name) [this] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	requires requires { overloaded_func_name(FWD(arg)...); }                                                                         \
	return overloaded_func_name(FWD(arg)...);                                                                                        \
}

#define AGE_IS_INVALID_ID(expr)	 (expr == age::get_invalid_id<BARE_OF(expr)>())
#define AGE_IS_INVALID_IDX(expr) (expr == age::get_invalid_id<BARE_OF(expr)>())

#define __AGE_GET_IMPL__(name, path)                                                  \
	struct                                                                            \
	{                                                                                 \
		FORCE_INLINE decltype(auto)                                                   \
		operator->() noexcept                                                         \
		{                                                                             \
			if constexpr (age::meta::cx_has_arrow<BARE_OF(global::detail::ctx.path)>) \
			{                                                                         \
				return global::detail::ctx.path;                                      \
			}                                                                         \
			else                                                                      \
			{                                                                         \
				return &global::detail::ctx.path;                                     \
			}                                                                         \
		}                                                                             \
		FORCE_INLINE auto&                                                            \
		operator()() noexcept                                                         \
		{                                                                             \
			return global::detail::ctx.path;                                          \
		}                                                                             \
		FORCE_INLINE                                                                  \
		operator auto&() noexcept                                                     \
		{                                                                             \
			return global::detail::ctx.path;                                          \
		}                                                                             \
		FORCE_INLINE decltype(auto)                                                   \
		operator[](auto&&... i) noexcept                                              \
		{                                                                             \
			return global::detail::ctx.path[FWD(i)...];                               \
		}                                                                             \
	} get_##name;

#define __AGE_SET_IMPL__(name, path)                  \
	struct                                            \
	{                                                 \
		FORCE_INLINE auto&                            \
		operator=(auto&& v) noexcept                  \
		{                                             \
			return global::detail::ctx.path = FWD(v); \
		}                                             \
		FORCE_INLINE auto&                            \
		operator()(auto&& v) noexcept                 \
		{                                             \
			return global::detail::ctx.path = FWD(v); \
		}                                             \
	} set_##name;

#define AGE_GET(name, path)	   __AGE_GET_IMPL__(name, path)
#define AGE_SET(name, path)	   __AGE_SET_IMPL__(name, path)
#define AGE_GETSET(name, path) __AGE_GET_IMPL__(name, path) __AGE_SET_IMPL__(name, path)

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

#define AGE_ENUM_STR_MAP_INSERT(name) m[#name] = __t_enum__::name;

#define AGE_DEFINE_ENUM(enum_class_name, underlying_type, ...)                           \
	enum class enum_class_name : underlying_type                                         \
	{                                                                                    \
		__VA_ARGS__                                                                      \
	};                                                                                   \
	constexpr inline std::string_view to_string(enum_class_name e) noexcept              \
	{                                                                                    \
		using __t_enum__ = enum_class_name;                                              \
		switch (e)                                                                       \
		{                                                                                \
			FOR_EACH (AGE_ENUM_TO_STRING_CASE, __VA_ARGS__)                              \
				;                                                                        \
		default:                                                                         \
		{                                                                                \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e));          \
		}                                                                                \
		}                                                                                \
	}                                                                                    \
	constexpr inline std::wstring_view to_wstring(enum_class_name e) noexcept            \
	{                                                                                    \
		using __t_enum__ = enum_class_name;                                              \
		switch (e)                                                                       \
		{                                                                                \
			FOR_EACH (AGE_ENUM_TO_WSTRING_CASE, __VA_ARGS__)                             \
				;                                                                        \
		default:                                                                         \
		{                                                                                \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e));          \
		}                                                                                \
		}                                                                                \
	}                                                                                    \
	template <typename t>                                                                \
	requires std::is_same_v<t, enum_class_name>                                          \
	consteval std::size_t size() noexcept                                                \
	{ return age::util::str_to_uint64(AGE_PP_STRINGIFY(AGE_PP_VA_COUNT(__VA_ARGS__))); } \
	constexpr FORCE_INLINE auto to_idx(enum_class_name e) noexcept                       \
	{ return std::to_underlying(e); }                                                    \
	constexpr inline std::size_t enum_class_name##_size = size<enum_class_name>();       \
	template <typename t>                                                                \
	requires std::is_same_v<t, enum_class_name>                                          \
	constexpr inline enum_class_name                                                     \
	str_to_enum(std::string_view sv) noexcept                                            \
	{                                                                                    \
		static const auto enum_class_name##_str_map = [] constexpr {                     \
			using __t_enum__ = enum_class_name;                                          \
			age::unordered_map<std::string_view, enum_class_name> m;                     \
			FOR_EACH (AGE_ENUM_STR_MAP_INSERT, __VA_ARGS__)                              \
				;                                                                        \
			return m;                                                                    \
		}();                                                                             \
		auto it = enum_class_name##_str_map.find(sv);                                    \
		AGE_ASSERT(it != enum_class_name##_str_map.end());                               \
		return it->second;                                                               \
	}                                                                                    \
                                                                                         \
	template <typename t, std::size_t n>                                                 \
	requires std::is_same_v<t, enum_class_name>                                          \
	constexpr enum_class_name                                                            \
	str_to_enum(const std::array<char, n>& arr) noexcept                                 \
	{                                                                                    \
		return str_to_enum<t>(std::string_view{ arr.data(), strnlen(arr.data(), n) });   \
	}


#define AGE_ENUM_DECL_VAL(tpl) AGE_PP_TUPLE_GET_0_I(tpl) = AGE_PP_TUPLE_GET_1_I(tpl)

#define AGE_ENUM_TO_STRING_CASE_VAL(tpl)			AGE_ENUM_TO_STRING_CASE_VAL_IMPL tpl
#define AGE_ENUM_TO_STRING_CASE_VAL_IMPL(name, val) AGE_ENUM_TO_STRING_CASE(name)

#define AGE_ENUM_TO_WSTRING_CASE_VAL(tpl)			 AGE_ENUM_TO_WSTRING_CASE_VAL_IMPL tpl
#define AGE_ENUM_TO_WSTRING_CASE_VAL_IMPL(name, val) AGE_ENUM_TO_WSTRING_CASE(name)

#define AGE_ENUM_STR_MAP_INSERT_VAL(tpl)			AGE_ENUM_STR_MAP_INSERT_VAL_IMPL tpl
#define AGE_ENUM_STR_MAP_INSERT_VAL_IMPL(name, val) m[#name] = __t_enum__::name;

#define AGE_DEFINE_ENUM_WITH_VALUE(enum_class_name, underlying_type, ...)                \
	enum class enum_class_name : underlying_type                                         \
	{                                                                                    \
		FOR_EACH_SEP(AGE_ENUM_DECL_VAL, AGE_PP_COMMA_I, __VA_ARGS__)                     \
	};                                                                                   \
	constexpr inline std::string_view to_string(enum_class_name e) noexcept              \
	{                                                                                    \
		using __t_enum__ = enum_class_name;                                              \
		switch (e)                                                                       \
		{                                                                                \
			FOR_EACH (AGE_ENUM_TO_STRING_CASE_VAL, __VA_ARGS__)                          \
			default:                                                                     \
			{                                                                            \
				AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e));      \
			}                                                                            \
		}                                                                                \
	}                                                                                    \
	constexpr inline std::wstring_view to_wstring(enum_class_name e) noexcept            \
	{                                                                                    \
		using __t_enum__ = enum_class_name;                                              \
		switch (e)                                                                       \
		{                                                                                \
			FOR_EACH (AGE_ENUM_TO_WSTRING_CASE_VAL, __VA_ARGS__)                         \
			default:                                                                     \
			{                                                                            \
				AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e));      \
			}                                                                            \
		}                                                                                \
	}                                                                                    \
	template <typename t>                                                                \
	requires std::is_same_v<t, enum_class_name>                                          \
	consteval std::size_t size() noexcept                                                \
	{ return age::util::str_to_uint64(AGE_PP_STRINGIFY(AGE_PP_VA_COUNT(__VA_ARGS__))); } \
	constexpr FORCE_INLINE auto to_idx(enum_class_name e) noexcept                       \
	{ return std::to_underlying(e); }                                                    \
	constexpr inline std::size_t enum_class_name##_size = size<enum_class_name>();       \
                                                                                         \
	template <typename t>                                                                \
	requires std::is_same_v<t, enum_class_name>                                          \
	constexpr inline enum_class_name                                                     \
	str_to_enum(std::string_view sv) noexcept                                            \
	{                                                                                    \
		constexpr static const auto enum_class_name##_str_map = [] constexpr {           \
			using __t_enum__ = enum_class_name;                                          \
			age::unordered_map<std::string_view, enum_class_name> m;                     \
			FOR_EACH (AGE_ENUM_STR_MAP_INSERT_VAL, __VA_ARGS__)                          \
				;                                                                        \
			return m;                                                                    \
		}();                                                                             \
		auto it = enum_class_name##_str_map.find(sv);                                    \
		AGE_ASSERT(it != enum_class_name##_str_map.end());                               \
		return it->second;                                                               \
	}                                                                                    \
                                                                                         \
	template <typename t, std::size_t n>                                                 \
	requires std::is_same_v<t, enum_class_name>                                          \
	constexpr enum_class_name                                                            \
	str_to_enum(const std::array<char, n>& arr) noexcept                                 \
	{                                                                                    \
		return str_to_enum<t>(std::string_view{ arr.data(), strnlen(arr.data(), n) });   \
	}


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

//---[ age_editor.hpp ]------------------------------------------------------------------
#define AGE_EDITOR_SCENE_ENTITY_STORAGES(...)                                                                                                        \
	FOR_EACH_SEP(AGE_EDITOR_SCENE_ENTITY_STORAGES_MAP_DECL, AGE_PP_EMPTY_I, __VA_ARGS__)                                                             \
	decltype(auto) storages() noexcept { return std::tie(FOR_EACH_ARG(AGE_PP_TUPLE_GET_1_I, __VA_ARGS__)); }                                         \
	decltype(auto) storages() const noexcept { return std::tie(FOR_EACH_ARG(AGE_PP_TUPLE_GET_1_I, __VA_ARGS__)); }                                   \
	static consteval decltype(auto) storage_names() noexcept { return std::tuple{ FOR_EACH_ARG(AGE_EDITOR_ENTITY_STORAGE_NAME_MAP, __VA_ARGS__) }; } \
	static consteval uint32 storage_count() { return static_cast<uint32>(std::tuple_size_v<BARE_OF(storage_names())>); }                             \
	void init() noexcept                                                                                                                             \
	{                                                                                                                                                \
		FOR_EACH (AGE_EDITOR_INIT_MAP, __VA_ARGS__)                                                                                                  \
			;                                                                                                                                        \
	}                                                                                                                                                \
	void deinit() noexcept                                                                                                                           \
	{                                                                                                                                                \
		FOR_EACH (AGE_EDITOR_DEINIT_MAP, __VA_ARGS__)                                                                                                \
			;                                                                                                                                        \
	}                                                                                                                                                \
	FORCE_INLINE decltype(auto) visit_storage_at(auto storage_idx, auto&& func, auto&&... arg) noexcept                                              \
	{                                                                                                                                                \
		return age::meta::visit_at(storages(), storage_idx, FWD(func), FWD(arg)...);                                                                 \
	}


#define AGE_EDITOR_SCENE_ENTITY_STORAGES_MAP_DECL(tpl)					AGE_EDITOR_SCENE_ENTITY_STORAGES_MAP_DECL_IMPL tpl
#define AGE_EDITOR_SCENE_ENTITY_STORAGES_MAP_DECL_IMPL(type, name, ...) type name;

#define AGE_EDITOR_ENTITY_STORAGE_NAME_MAP(tpl)					 AGE_EDITOR_ENTITY_STORAGE_NAME_MAP_IMPL tpl
#define AGE_EDITOR_ENTITY_STORAGE_NAME_MAP_IMPL(type, name, ...) age::util::to_fixed_str_arr<age::config::max_entity_storage_name_len>(#name, __VA_ARGS__)


#define AGE_EDITOR_GAME_NAME(...) \
	static consteval decltype(auto) age_editor_name() { return age::util::to_fixed_str_arr<age::config::max_game_name_len>(__VA_ARGS__); }

#define AGE_EDITOR_GAME_SCENES(...)                                                                                                                                                                               \
	FOR_EACH_SEP(AGE_EDITOR_GAME_SCENES_MAP_DECL, AGE_PP_EMPTY_I, __VA_ARGS__)                                                                                                                                    \
	decltype(auto) scenes() noexcept { return std::tie(FOR_EACH_ARG(AGE_PP_TUPLE_GET_1_I, __VA_ARGS__)); }                                                                                                        \
	decltype(auto) scenes() const noexcept { return std::tie(FOR_EACH_ARG(AGE_PP_TUPLE_GET_1_I, __VA_ARGS__)); }                                                                                                  \
	static consteval decltype(auto) scene_names() noexcept { return std::tuple{ FOR_EACH_ARG(AGE_EDITOR_SCENE_NAME_MAP, __VA_ARGS__) }; }                                                                         \
	static consteval uint32 scene_count() { return static_cast<uint32>(std::tuple_size_v<BARE_OF(scene_names())>); }                                                                                              \
	void init() noexcept                                                                                                                                                                                          \
	{                                                                                                                                                                                                             \
		FOR_EACH (AGE_EDITOR_INIT_MAP, __VA_ARGS__)                                                                                                                                                               \
			;                                                                                                                                                                                                     \
	}                                                                                                                                                                                                             \
	void deinit() noexcept                                                                                                                                                                                        \
	{                                                                                                                                                                                                             \
		FOR_EACH (AGE_EDITOR_DEINIT_MAP, __VA_ARGS__)                                                                                                                                                             \
			;                                                                                                                                                                                                     \
	}                                                                                                                                                                                                             \
	FORCE_INLINE decltype(auto) visit_scene_at(auto scene_idx, auto&& func, auto&&... arg) noexcept                                                                                                               \
	{                                                                                                                                                                                                             \
		return age::meta::visit_at(scenes(), scene_idx, FWD(func), FWD(arg)...);                                                                                                                                  \
	}                                                                                                                                                                                                             \
	FORCE_INLINE decltype(auto) visit_storage_at(auto scene_idx, auto storage_idx, auto&& func, auto&&... arg) noexcept                                                                                           \
	{                                                                                                                                                                                                             \
		return visit_scene_at(scene_idx, AGE_LAMBDA((auto&& scene, auto idx, auto&& func, auto&&... arg), { return scene.visit_storage_at(idx, FWD(func), FWD(arg)...); }), storage_idx, FWD(func), FWD(arg)...); \
	}                                                                                                                                                                                                             \
	FORCE_INLINE void visit_all_scenes(auto&& func, auto&&... arg) noexcept                                                                                                                                       \
	{                                                                                                                                                                                                             \
		for (auto i = 0u; i < scene_count(); ++i)                                                                                                                                                                 \
		{                                                                                                                                                                                                         \
			visit_scene_at(i, FWD(func), FWD(arg)...);                                                                                                                                                            \
		}                                                                                                                                                                                                         \
	}                                                                                                                                                                                                             \
	FORCE_INLINE void visit_all_storages(auto&& func, auto&&... arg) noexcept                                                                                                                                     \
	{                                                                                                                                                                                                             \
		for (auto i = 0u; i < scene_count(); ++i)                                                                                                                                                                 \
		{                                                                                                                                                                                                         \
			visit_scene_at(i, AGE_LAMBDA((auto&& scene, auto&& func, auto&&... arg), {                                                                                                                            \
							   for (auto j = 0u; j < scene.storage_count(); ++j)                                                                                                                                  \
							   {                                                                                                                                                                                  \
								   scene.visit_storage_at(j, FWD(func), FWD(arg)...);                                                                                                                             \
							   }                                                                                                                                                                                  \
						   }),                                                                                                                                                                                    \
						   FWD(func), FWD(arg)...);                                                                                                                                                               \
		}                                                                                                                                                                                                         \
	}


#define AGE_EDITOR_GAME_SCENES_MAP_DECL(tpl)				  AGE_EDITOR_GAME_SCENES_MAP_DECL_IMPL tpl
#define AGE_EDITOR_GAME_SCENES_MAP_DECL_IMPL(type, name, ...) type name;

#define AGE_EDITOR_SCENE_NAME_MAP(tpl)					AGE_EDITOR_SCENE_NAME_MAP_IMPL tpl
#define AGE_EDITOR_SCENE_NAME_MAP_IMPL(type, name, ...) age::util::to_fixed_str_arr<age::config::max_scene_name_len>(#name, __VA_ARGS__)

#define AGE_EDITOR_INIT_MAP(tpl)				  AGE_EDITOR_INIT_MAP_IMPL tpl
#define AGE_EDITOR_INIT_MAP_IMPL(type, name, ...) name.init()

#define AGE_EDITOR_DEINIT_MAP(tpl)					AGE_EDITOR_DEINIT_MAP_IMPL tpl
#define AGE_EDITOR_DEINIT_MAP_IMPL(type, name, ...) name.deinit()
//---------------------------------------------------------------------------------------