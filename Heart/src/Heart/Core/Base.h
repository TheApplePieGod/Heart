#pragma once

#ifdef HE_DEBUG
	#if defined(HT_PLATFORM_WINDOWS)
		#define HE_DEBUGBREAK() __debugbreak()
	#elif defined(HT_PLATFORM_LINUX)
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

namespace Heart
{
    typedef signed char s8;
    typedef short s16;
    typedef int s32;
    typedef long long s64;
    typedef int b32;
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;
    typedef unsigned long long u64;
    typedef float f32;
    typedef double d64;

    #define Pi32 3.14159265359f
    #define Tau32 6.28318530717958647692f
    #define Kilobytes(Value) ((Value)*1024LL)
    #define Megabytes(Value) (Kilobytes(Value)*1024LL)
    #define Gigabytes(Value) (Megabytes(Value)*1024LL)
    #define Terabytes(Value) (Gigabytes(Value)*1024LL)
    #define DegreesToRadians(Degrees) Degrees*Pi32/180.f

    #define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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