#pragma once

#ifdef HE_DEBUG
	#if defined(_MSC_VER)
		#define HE_DEBUGBREAK() __debugbreak()
	#elif defined(__clang__)
		#if __has_builtin(__builtin_debugtrap)
			#define HE_DEBUGBREAK() __builtin_debugtrap()
		#else
			#define HE_DEBUGBREAK() __builtin_trap()
		#endif
	#elif defined(__GNUC__)
		#define HE_DEBUGBREAK() __builtin_trap()
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define HE_ENABLE_ASSERTS
#else
	#define HE_DEBUGBREAK()
#endif

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using b32 = int32_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float_t;
using d64 = double_t;
using uptr = intptr_t;
using wchar = wchar_t;
using char8 = char;
using char16 = char16_t;
using byte = unsigned char;

#define HE_ENUM_TO_STRING(class, value) class::TypeStrings[static_cast<u16>(value)]
#define HE_EXPAND_ARGS(args) args
#define HE_ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])
#define HE_BIT(x) (1 << x)
#define HE_PLACEMENT_NEW(ptr, type, ...) new (ptr) type(__VA_ARGS__)

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

	// A 'WeakRef' is similar to a 'Ref,' except it will not keep the pointed to object alive
	// and it must be created from a 'Ref'
	template<typename T>
	using WeakRef = std::weak_ptr<T>;
}
