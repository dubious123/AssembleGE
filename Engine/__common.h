#pragma once
#include <utility>
#include "__math.h"

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

#if defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define FORCE_INLINE inline __attribute__((always_inline))
#else
	#define FORCE_INLINE inline
#endif

#if defined(_MSC_VER)
	#define INLINE_LAMBDA_FRONT 
	#define INLINE_LAMBDA_BACK [[msvc::forceinline]]
#elif defined(__GNUC__) || defined(__clang__)
	#define INLINE_LAMBDA_FRONT __attribute__((always_inline))
	#define INLINE_LAMBDA_BACK	
#else
	#define INLINE_LAMBDA
#endif

#define FWD(x) std::forward<decltype(x)>(x)

constexpr std::size_t
operator"" _KiB(std::size_t k)
{
	return k * 1024;
}
