#pragma once

#if defined(HE_DEBUG) && !defined(HE_DIST)

#include "optick.h"

#define HE_PROFILE_FUNCTION() OPTICK_EVENT()
#define HE_PROFILE_FRAME() OPTICK_FRAME("Main Thread")
#define HE_PROFILE_THREAD(name) OPTICK_THREAD(name)

#else

#define HE_PROFILE_FUNCTION()
#define HE_PROFILE_FRAME()
#define HE_PROFILE_THREAD(name)

#endif

// Undefine optick's definition here because it gets redefined with the vulkansdk 
#undef VKAPI_PTR