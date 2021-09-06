#pragma once

#ifdef HE_DEBUG
	#if defined(HE_PLATFORM_WINDOWS)
		#define HE_DEBUGBREAK() __debugbreak()
	#elif defined(HE_PLATFORM_LINUX)
		#include <signal.h>
		#define HE_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define HE_ENABLE_ASSERTS
#else
	#define HE_DEBUGBREAK()
#endif

#define HE_EXPAND_ARGS(args) args
#define BIT(x) (1 << x)

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef int32_t b32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float_t f32;
typedef double_t d64;

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define DegreesToRadians(Degrees) Degrees*Pi32/180.f

namespace Heart
{
    // A 'Scope' variable is one which has its lifetime managed by the owning object
    // when the object dies, so does the variable 
    template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

    // A 'Ref' variable is one which stays alive for as long as something is referencing it
    // when all referencing objects die, so does the variable 
	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}