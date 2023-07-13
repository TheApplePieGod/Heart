#pragma once

#if defined(HE_DEBUG) && !defined(HE_DIST)

#include "tracy/Tracy.hpp"

#define HE_PROFILE_FUNCTION() ZoneScoped
#define HE_PROFILE_FRAME() FrameMark
#define HE_PROFILE_THREAD(name)

#else

#define HE_PROFILE_FUNCTION()
#define HE_PROFILE_FRAME()
#define HE_PROFILE_THREAD(name)

#endif
