#pragma once

#ifdef HT_ENABLE_ASSERTS

    #define HT_CRASH_ASSERT(expression) if(!(expression)) {*(int *)0 = 0;}
    #define HT_INTERNAL_ASSERT_IMPL(check, msg, ...) { if (!(check)) { HT_ENGINE_LOG_ERROR(msg, __VA_ARGS__); HT_DEBUGBREAK(); } }
    #define HT_INTERNAL_ASSERT_MSG(check, ...) HT_INTERNAL_ASSERT_IMPL(check, "Assertion failed: {0}", __VA_ARGS__)
    #define HT_INTERNAL_ASSERT_NOMSG(check) HT_INTERNAL_ASSERT_IMPL(check, "Assertion '{0}' failed at {1}:{2}", #check, std::filesystem::path(__FILE__).filename().string(), __LINE__)

    #define HT_INTERNAL_ASSERT_FIND(_1, _2, macro, ...) macro
    #define HT_INTERNAL_ASSERT_PICK(...) HT_EXPAND_ARGS( HT_INTERNAL_ASSERT_FIND(__VA_ARGS__, HT_INTERNAL_ASSERT_MSG, HT_INTERNAL_ASSERT_NOMSG) )

    #define HT_ENGINE_ASSERT(...) HT_EXPAND_ARGS( HT_INTERNAL_ASSERT_PICK(__VA_ARGS__)(__VA_ARGS__) )

#else

    #define HT_ENGINE_ASSERT(...)

#endif