#ifdef HE_PLATFORM_ANDROID

#include <native_app_glue/android_native_app_glue.h>

namespace Heart
{
    struct AndroidApp
    {
        inline static android_app* App = nullptr;
    };
}

#endif
