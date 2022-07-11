#pragma once

#ifdef HE_ENABLE_ASSERTS

    #define HE_CRASH_ASSERT(expression) if(!(expression)) {*(int *)0 = 0;}
    #define HE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if (!(check)) { HE_##type##_LOG_ERROR(msg, __VA_ARGS__); HE_DEBUGBREAK(); } }
    #define HE_INTERNAL_ASSERT_MSG(type, check, ...) HE_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
    #define HE_INTERNAL_ASSERT_NOMSG(type, check) HE_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", #check, std::filesystem::path(__FILE__).filename().string(), __LINE__)

    #define HE_INTERNAL_ASSERT_FIND(_1, _2, macro, ...) macro
    #define HE_INTERNAL_ASSERT_PICK(...) HE_EXPAND_ARGS( HE_INTERNAL_ASSERT_FIND(__VA_ARGS__, HE_INTERNAL_ASSERT_MSG, HE_INTERNAL_ASSERT_NOMSG) )

    #define HE_ENGINE_ASSERT(...) HE_EXPAND_ARGS( HE_INTERNAL_ASSERT_PICK(__VA_ARGS__)(ENGINE, __VA_ARGS__) )
    #define HE_ASSERT(...) HE_EXPAND_ARGS( HE_INTERNAL_ASSERT_PICK(__VA_ARGS__)(CLIENT, __VA_ARGS__) )

#else

    #define HE_ENGINE_ASSERT(...)
    #define HE_ASSERT(...)

#endif