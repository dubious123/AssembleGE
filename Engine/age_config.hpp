#pragma once
#if defined(_WIN32) || defined(_WIN64)
	#define AGE_PLATFORM_WINDOW
#endif

#if defined _DEBUG
	#define AGE_DEBUG
#endif

#if defined NDEBUG
	#define AGE_RELEASE
#endif

#define AGE_GRAPHICS_BACKEND_DX12

#if defined _MSC_VER
	#define AGE_COMPILER_MSVC
#elif defined __GNUC__
	#define AGE_COMPILER_GCC
#elif defined __clang__
	#define AGE_COMPILER_CLANG
#endif

#if defined(_MSC_VER)
	#if defined(_M_X64)
		#define AGE_ARCH_X64 1
	#elif defined(_M_ARM64)
		#define AGE_ARCH_ARM64 1
	#elif defined(_M_IX86)
		#define AGE_ARCH_X86 1
	#elif defined(_M_ARM)
		#define AGE_ARCH_ARM32 1
	#else
		#error "Unknown MSVC architecture"
	#endif
#else
	#if defined(__x86_64__) || defined(__amd64__)
		#define AGE_ARCH_X64 1
	#elif defined(__aarch64__)
		#define AGE_ARCH_ARM64 1
	#elif defined(__i386__)
		#define AGE_ARCH_X86 1
	#elif defined(__arm__)
		#define AGE_ARCH_ARM32 1
	#else
		#error "Unknown architecture"
	#endif
#endif

#ifdef AGE_COMPILER_MSVC
	#define no_unique_addr [[no_unique_address]] [[msvc::no_unique_address]]
#else
	#define no_unique_addr [[no_unique_address]]
#endif

#if defined(AGE_COMPILER_MSVC)
	#pragma warning(error : 4714)
	#define FORCE_INLINE __forceinline
#elif defined(AGE_COMPILER_GCC)
	#define FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline
#elif defined(AGE_COMPILER_CLANG)
	#define FORCE_INLINE inline __attribute__((always_inline))
#else
	#define FORCE_INLINE inline
#endif

#if defined(AGE_COMPILER_MSVC)
	#define INLINE_LAMBDA_FRONT
	#define INLINE_LAMBDA_BACK [[msvc::forceinline]]
#elif defined AGE_COMPILER_GCC || defined(__clang__)
	#define INLINE_LAMBDA_FRONT __attribute__((always_inline))
	#define INLINE_LAMBDA_BACK
#else
	#define INLINE_LAMBDA_FRONT
	#define INLINE_LAMBDA_BACK
#endif

#define AGE_SIMD_CALL XM_CALLCONV	 // todo

#ifdef AGE_COMPILER_MSVC
	#define AGE_ALLOC_CALLCONV __cdecl
#else
	#define AGE_ALLOC_CALLCONV
#endif

#ifndef AGE_EXPORT
	#if defined _WIN32 || defined __CYGWIN__ || defined AGE_COMPILER_MSVC
		#define AGE_EXPORT __declspec(dllexport)
		#define AGE_IMPORT __declspec(dllimport)
		#define AGE_HIDDEN
	#elif defined AGE_COMPILER_GCC && __GNUC__ >= 4
		#define AGE_EXPORT __attribute__((visibility("default")))
		#define AGE_IMPORT __attribute__((visibility("default")))
		#define AGE_HIDDEN __attribute__((visibility("hidden")))
	#else /* Unsupported compiler */
		#define AGE_EXPORT
		#define AGE_IMPORT
		#define AGE_HIDDEN
	#endif
#endif

#ifndef AGE_API
	#if defined AGE_API_EXPORT
		#define AGE_API AGE_EXPORT
	#elif defined AGE_API_IMPORT
		#define AGE_API AGE_IMPORT
	#else /* No API */
		#define AGE_API
	#endif
#endif

// #define USE_STL_VECTOR
#define USE_STL_SET
#define USE_STL_LIST
#define USE_STL_MAP

namespace age::config
{
	inline constexpr bool debug_mode =
#if defined(AGE_DEBUG)
		true;
#else
		false;
#endif
	inline constexpr bool release_mode =
#if defined(AGE_RELEASE)
		true;
#else
		false;
#endif

	inline constexpr unsigned short version_major = 0;
	inline constexpr unsigned short version_minor = 0;
}	 // namespace age::config