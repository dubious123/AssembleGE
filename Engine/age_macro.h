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

#if defined(__cpp_multidimensional_subscript)
	#define AGE_MD_INDEX(...) __VA_ARGS__
#else
	#define AGE_MD_INDEX(...) \
		std::array { __VA_ARGS__ }
#endif

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

#define AGE_FUNC(...) [] INLINE_LAMBDA_FRONT(auto&&... arg) noexcept INLINE_LAMBDA_BACK -> decltype(auto) { \
	return __VA_ARGS__(FWD(arg)...);                                                                        \
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
			FOR_EACH(AGE_ENUM_TO_STRING_CASE, __VA_ARGS__);                     \
		default:                                                                \
		{                                                                       \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
		}                                                                       \
		}                                                                       \
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
			FOR_EACH(AGE_ENUM_TO_STRING_CASE, __VA_ARGS__);                     \
		default:                                                                \
		{                                                                       \
			AGE_UNREACHABLE("invalid enum, value : {}", std::to_underlying(e)); \
		}                                                                       \
		}                                                                       \
	}

//---[ age_request.hpp ]------------------------------------------------------------
#define __AGE_REQUEST_COMBINE_FLAGS__(x) age::subsystem::flags::x |

#define AGE_REQUEST_PHASE(...) \
	phase_meta<FOR_EACH(__AGE_REQUEST_COMBINE_FLAGS__, __VA_ARGS__) age::subsystem::flags{ 0 }>

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
//----------------------------------------------------------------------------------

//---[ age_graphics_backend_dx12.hpp ]------------------------------------------------------------
#define __AGE_VALIDATE_EXTRACT_VIEW_DESC_TYPE_IMPL__(_0, _1, view_desc) decltype(view_desc)
#define __AGE_VALIDATE_EXTRACT_VIEW_DESC_TYPE__(expression)				__AGE_VALIDATE_EXTRACT_VIEW_DESC_TYPE_IMPL__ expression
#define __AGE_VALIDATE_HELPER__(...) \
	using __t_view_desc_type_pack__ = age::meta::type_pack<FOR_EACH_ARG(__AGE_VALIDATE_EXTRACT_VIEW_DESC_TYPE__ __VA_OPT__(, ) __VA_ARGS__)>;

#define AGE_VALIDATE_DIMENSION(expected) AGE_ASSERT(__age_resource_desc__.Dimension == expected, "[resource validate] invalid dimension, expected : {}, actual : {}", #expected, to_string(__age_resource_desc__.Dimension));
#define AGE_RESOURCE_VIEW_VALIDATE(...)	 AGE_DO_WHILE(__VA_ARGS__);

#define AGE_DESC_HANDLE_MEMBER_CBV(h_desc_member_name, ...)		(age::graphics::cbv_desc_handle, h_desc_member_name, (__VA_ARGS__))
#define AGE_DESC_HANDLE_MEMBER_SRV(h_desc_member_name, ...)		(age::graphics::srv_desc_handle, h_desc_member_name, (__VA_ARGS__))
#define AGE_DESC_HANDLE_MEMBER_UAV(h_desc_member_name, ...)		(age::graphics::uav_desc_handle, h_desc_member_name, (__VA_ARGS__))
#define AGE_DESC_HANDLE_MEMBER_RTV(h_desc_member_name, ...)		(age::graphics::rtv_desc_handle, h_desc_member_name, (__VA_ARGS__))
#define AGE_DESC_HANDLE_MEMBER_DSV(h_desc_member_name, ...)		(age::graphics::dsv_desc_handle, h_desc_member_name, (__VA_ARGS__))
#define AGE_DESC_HANDLE_MEMBER_SAMPLER(h_desc_member_name, ...) (age::graphics::sampler_desc_handle, h_desc_member_name, (__VA_ARGS__))

#define __AGE_GRAPHICS_RESOURCE_VIEW_DEFINE_MEMBER_AND_GET_DESC_FUNC_IMPL__(h_desc_type, h_desc_member_name, desc) \
  public:                                                                                                          \
	h_desc_type h_desc_member_name{};                                                                              \
                                                                                                                   \
	FORCE_INLINE static constexpr decltype(auto)                                                                   \
	h_desc_member_name##_view_desc() noexcept                                                                      \
	{                                                                                                              \
		return desc;                                                                                               \
	}

#define __AGE_GRAPHICS_RESOURCE_VIEW_DEFINE_MEMBER_AND_GET_DESC_FUNC__(expression) __AGE_GRAPHICS_RESOURCE_VIEW_DEFINE_MEMBER_AND_GET_DESC_FUNC_IMPL__ expression

#define __AGE_GRAPHICS_RESOURCE_VIEW_INIT_IMPL__(_0, h_desc_member_name, _2, ...)	age::graphics::pop_descriptor(h_desc_member_name);
#define __AGE_GRAPHICS_RESOURCE_VIEW_INIT__(expression)								__AGE_GRAPHICS_RESOURCE_VIEW_INIT_IMPL__ expression
#define __AGE_GRAPHICS_RESOURCE_VIEW_BIND_IMPL__(_0, h_desc_member_name, _2, ...)	age::graphics::resource::create_view(__age_resource__, h_desc_member_name, h_desc_member_name##_view_desc());
#define __AGE_GRAPHICS_RESOURCE_VIEW_BIND__(expression)								__AGE_GRAPHICS_RESOURCE_VIEW_BIND_IMPL__ expression
#define __AGE_GRAPHICS_RESOURCE_VIEW_DEINIT_IMPL__(_0, h_desc_member_name, _2, ...) age::graphics::push_descriptor(h_desc_member_name);
#define __AGE_GRAPHICS_RESOURCE_VIEW_DEINIT__(expression)							__AGE_GRAPHICS_RESOURCE_VIEW_DEINIT_IMPL__ expression

#define __AGE_WSTR_IMPL__(x) L##x
#define __AGE_WSTR__(x)		 __AGE_WSTR_IMPL__(x)

#define __AGE_DEFINE_RESOURCE_VIEW_IMPL__(view_type_name, view_local_name, validation, ...)                                                                                                               \
	struct view_type_name                                                                                                                                                                                 \
	{                                                                                                                                                                                                     \
		const ID3D12Resource* p_resource = nullptr;                                                                                                                                                       \
		__AGE_VALIDATE_HELPER__(__VA_ARGS__)                                                                                                                                                              \
		FOR_EACH(__AGE_GRAPHICS_RESOURCE_VIEW_DEFINE_MEMBER_AND_GET_DESC_FUNC__ __VA_OPT__(, ) __VA_ARGS__);                                                                                              \
                                                                                                                                                                                                          \
	  public:                                                                                                                                                                                             \
		static constexpr std::string_view                                                                                                                                                                 \
		debug_name() noexcept                                                                                                                                                                             \
		{                                                                                                                                                                                                 \
			return #view_type_name #view_local_name;                                                                                                                                                      \
		}                                                                                                                                                                                                 \
                                                                                                                                                                                                          \
		static constexpr std::wstring_view                                                                                                                                                                \
		debug_name_wstr() noexcept                                                                                                                                                                        \
		{                                                                                                                                                                                                 \
			return __AGE_WSTR__(#view_type_name #view_local_name);                                                                                                                                        \
		}                                                                                                                                                                                                 \
		static constexpr void                                                                                                                                                                             \
		validate(ID3D12Resource& __age_resource__) noexcept                                                                                                                                               \
		{                                                                                                                                                                                                 \
			if constexpr (age::config::debug_mode)                                                                                                                                                        \
			{                                                                                                                                                                                             \
				const auto __age_resource_desc__ = __age_resource__.GetDesc();                                                                                                                            \
                                                                                                                                                                                                          \
				AGE_ASSERT(__age_resource_desc__.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN);                                                                                                          \
                                                                                                                                                                                                          \
				constexpr auto __age_val_type__ = []<typename t_view_desc>(const auto& desc) {                                                                                                            \
					if constexpr (std::is_same_v<t_view_desc, D3D12_CONSTANT_BUFFER_VIEW_DESC>)                                                                                                           \
					{                                                                                                                                                                                     \
						AGE_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER,                                                                                                                     \
								   "[resource validate] invalid dimension, expected BUFFER (required by CBV)");                                                                                           \
					}                                                                                                                                                                                     \
					else if constexpr (std::is_same_v<t_view_desc, D3D12_SHADER_RESOURCE_VIEW_DESC>)                                                                                                      \
					{                                                                                                                                                                                     \
						AGE_ASSERT((desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0,                                                                                                          \
								   "[resource validate] invalid flags, forbidden flag DENY_SHADER_RESOURCE (SRV present)");                                                                               \
					}                                                                                                                                                                                     \
					else if constexpr (std::is_same_v<t_view_desc, D3D12_UNORDERED_ACCESS_VIEW_DESC>)                                                                                                     \
					{                                                                                                                                                                                     \
						AGE_ASSERT(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,                                                                                                               \
								   "[resource validate] invalid flags, missing flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS (required by UAV)");                                                       \
						AGE_ASSERT(desc.SampleDesc.Count == 1,                                                                                                                                            \
								   "[resource validate] invalid sample count, MSAA resource is not allowed for UAV");                                                                                     \
					}                                                                                                                                                                                     \
					else if constexpr (std::is_same_v<t_view_desc, D3D12_RENDER_TARGET_VIEW_DESC>)                                                                                                        \
					{                                                                                                                                                                                     \
						AGE_ASSERT(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,                                                                                                                  \
								   "[resource validate] invalid flags, missing flag D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET (required by RTV)");                                                          \
						AGE_ASSERT(desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER,                                                                                                                     \
								   "[resource validate] invalid dimension, expected TEXTURE resource (required by RTV)");                                                                                 \
						switch (desc.Format)                                                                                                                                                              \
						{                                                                                                                                                                                 \
						case DXGI_FORMAT_R16_TYPELESS:                                                                                                                                                    \
						case DXGI_FORMAT_R24G8_TYPELESS:                                                                                                                                                  \
						case DXGI_FORMAT_R32_TYPELESS:                                                                                                                                                    \
						case DXGI_FORMAT_R32G8X24_TYPELESS:                                                                                                                                               \
						case DXGI_FORMAT_D16_UNORM:                                                                                                                                                       \
						case DXGI_FORMAT_D24_UNORM_S8_UINT:                                                                                                                                               \
						case DXGI_FORMAT_D32_FLOAT:                                                                                                                                                       \
						case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:                                                                                                                                            \
						{                                                                                                                                                                                 \
							AGE_ASSERT(false, "[resource validate] invalid format, resource format is not compatible with RTV, actual: {}", to_string(desc.Format));                                      \
							break;                                                                                                                                                                        \
						}                                                                                                                                                                                 \
						default:                                                                                                                                                                          \
						{                                                                                                                                                                                 \
							break;                                                                                                                                                                        \
						}                                                                                                                                                                                 \
						}                                                                                                                                                                                 \
					}                                                                                                                                                                                     \
					else if constexpr (std::is_same_v<t_view_desc, D3D12_DEPTH_STENCIL_VIEW_DESC>)                                                                                                        \
					{                                                                                                                                                                                     \
						AGE_ASSERT(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,                                                                                                                  \
								   "[resource validate] invalid flags, missing flag ALLOW_DEPTH_STENCIL (required by DSV)");                                                                              \
                                                                                                                                                                                                          \
						AGE_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D,                                                                                                                  \
								   "[resource validate] invalid dimension, expected TEXTURE2D (required by DSV)");                                                                                        \
                                                                                                                                                                                                          \
						switch (desc.Format)                                                                                                                                                              \
						{                                                                                                                                                                                 \
						case DXGI_FORMAT_R16_TYPELESS:                                                                                                                                                    \
						case DXGI_FORMAT_R24G8_TYPELESS:                                                                                                                                                  \
						case DXGI_FORMAT_R32_TYPELESS:                                                                                                                                                    \
						case DXGI_FORMAT_R32G8X24_TYPELESS:                                                                                                                                               \
						case DXGI_FORMAT_D16_UNORM:                                                                                                                                                       \
						case DXGI_FORMAT_D24_UNORM_S8_UINT:                                                                                                                                               \
						case DXGI_FORMAT_D32_FLOAT:                                                                                                                                                       \
						case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:                                                                                                                                            \
						{                                                                                                                                                                                 \
							break;                                                                                                                                                                        \
						}                                                                                                                                                                                 \
						default:                                                                                                                                                                          \
						{                                                                                                                                                                                 \
							AGE_ASSERT(false, "[resource validate] invalid format, resource format is not compatible with DSV (expected depth/stencil or typeless), actual: {}", to_string(desc.Format)); \
						}                                                                                                                                                                                 \
						}                                                                                                                                                                                 \
					}                                                                                                                                                                                     \
				};                                                                                                                                                                                        \
				[__age_val_type__]<typename... t_view_desc>(age::meta::type_pack<t_view_desc...>, const auto& desc) {                                                                                     \
					(__age_val_type__.operator()<t_view_desc>(desc), ...);                                                                                                                                \
				}(__t_view_desc_type_pack__{}, __age_resource_desc__);                                                                                                                                    \
                                                                                                                                                                                                          \
				validation                                                                                                                                                                                \
			}                                                                                                                                                                                             \
		}                                                                                                                                                                                                 \
		void                                                                                                                                                                                              \
		init() noexcept                                                                                                                                                                                   \
		{                                                                                                                                                                                                 \
			FOR_EACH(__AGE_GRAPHICS_RESOURCE_VIEW_INIT__ __VA_OPT__(, ) __VA_ARGS__);                                                                                                                     \
		}                                                                                                                                                                                                 \
		void                                                                                                                                                                                              \
		bind(ID3D12Resource& __age_resource__) noexcept                                                                                                                                                   \
		{                                                                                                                                                                                                 \
			p_resource = &__age_resource__;                                                                                                                                                               \
			validate(__age_resource__);                                                                                                                                                                   \
			FOR_EACH(__AGE_GRAPHICS_RESOURCE_VIEW_BIND__ __VA_OPT__(, ) __VA_ARGS__);                                                                                                                     \
		}                                                                                                                                                                                                 \
		void                                                                                                                                                                                              \
		deinit() noexcept                                                                                                                                                                                 \
		{                                                                                                                                                                                                 \
			if constexpr (age::config::debug_mode)                                                                                                                                                        \
			{                                                                                                                                                                                             \
				p_resource = nullptr;                                                                                                                                                                     \
			}                                                                                                                                                                                             \
			FOR_EACH(__AGE_GRAPHICS_RESOURCE_VIEW_DEINIT__ __VA_OPT__(, ) __VA_ARGS__);                                                                                                                   \
		}                                                                                                                                                                                                 \
	} view_local_name;

#define AGE_DEFINE_RESOURCE_VIEW(view_type_name, validation, ...)		 __AGE_DEFINE_RESOURCE_VIEW_IMPL__(view_type_name, , validation, __VA_ARGS__)
#define AGE_DEFINE_LOCAL_RESOURCE_VIEW(view_local_name, validation, ...) __AGE_DEFINE_RESOURCE_VIEW_IMPL__(, view_local_name, validation, __VA_ARGS__)

#define AGE_DEFINE_RESOURCE_FLOW(...) __VA_ARGS__
#define AGE_RESOURCE_BARRIER_ARR(...) \
	AGE_PP_FIRST_I(__VA_OPT__(AGE_PP_IDENTITY_I(const auto __resource_barrier_arr__ = std::to_array<D3D12_RESOURCE_BARRIER>({ __VA_ARGS__ })), ) AGE_PP_IDENTITY_I(const auto __resource_barrier_arr__ = std::array<D3D12_RESOURCE_BARRIER, 0>{}))
#define AGE_RENDER_PASS_RT_DESC_ARR(...) \
	AGE_PP_FIRST_I(__VA_OPT__(AGE_PP_IDENTITY_I(const auto __render_pass_rt_desc_arr__ = std::to_array<D3D12_RENDER_PASS_RENDER_TARGET_DESC>({ __VA_ARGS__ })), ) AGE_PP_IDENTITY_I(const auto __render_pass_rt_desc_arr__ = std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, 0>{}))
#define AGE_RENDER_PASS_DS_DESC(...) \
	AGE_PP_FIRST_I(__VA_OPT__(AGE_PP_IDENTITY_I(const auto __render_pass_ds_desc_arr__ = std::to_array<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC>({ __VA_ARGS__ })), ) AGE_PP_IDENTITY_I(const auto __render_pass_ds_desc_arr__ = std::array<D3D12_RENDER_PASS_DEPTH_STENCIL_DESC, 0>{}))

#define AGE_RESOURCE_FLOW_PHASE(phase_name, resource_barrier_arr, render_pass_rt_desc_arr, render_pass_ds_desc, ...)                                                                                                                  \
	FORCE_INLINE decltype(auto)                                                                                                                                                                                                       \
	phase_name(t_igraphics_cmd_list& __age_cmd_list__) noexcept                                                                                                                                                                       \
	{                                                                                                                                                                                                                                 \
		resource_barrier_arr;                                                                                                                                                                                                         \
		render_pass_rt_desc_arr;                                                                                                                                                                                                      \
		render_pass_ds_desc;                                                                                                                                                                                                          \
		static_assert(std::size(__render_pass_ds_desc_arr__) <= 1, "render pass can have at most one depth-stencil attachment");                                                                                                      \
                                                                                                                                                                                                                                      \
		if constexpr (std::size(__resource_barrier_arr__) > 0)                                                                                                                                                                        \
		{                                                                                                                                                                                                                             \
			__age_cmd_list__.ResourceBarrier(std::size(__resource_barrier_arr__), __resource_barrier_arr__.data());                                                                                                                   \
		}                                                                                                                                                                                                                             \
                                                                                                                                                                                                                                      \
		if constexpr (std::size(__render_pass_rt_desc_arr__) + std::size(__render_pass_ds_desc_arr__) > 0)                                                                                                                            \
		{                                                                                                                                                                                                                             \
			__age_cmd_list__.BeginRenderPass(std::size(__render_pass_rt_desc_arr__), __render_pass_rt_desc_arr__.data(), __render_pass_ds_desc_arr__.data(), AGE_PP_FIRST_I(__VA_ARGS__ __VA_OPT__(, ) D3D12_RENDER_PASS_FLAG_NONE)); \
			return age::util::scope_guard{ [&__age_cmd_list__] INLINE_LAMBDA_FRONT() noexcept INLINE_LAMBDA_BACK { __age_cmd_list__.EndRenderPass(); } };                                                                             \
		}                                                                                                                                                                                                                             \
	}

//------------------------------------------------------------------------------------------------