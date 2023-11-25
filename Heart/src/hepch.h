#pragma once

#include "Heart/Core/PlatformDetection.h"

#ifdef HE_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <optional>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <random>
#include <thread>
#include <type_traits>
#include <new>
#include <locale>
#include <atomic>
#include <condition_variable>

#include <string>
#include <regex>
#include <array>
#include <vector>
#include <queue>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

#include "Heart/Core/Base.h"
#include "Heart/Core/Log.h"
#include "Heart/Core/Assert.h"
#include "Heart/Core/Profile.h"

#ifdef HE_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <ShlObj.h>
	#include <comdef.h>
#elif defined(HE_PLATFORM_LINUX)
    #include <dlfcn.h>
#elif defined(HE_PLATFORM_MACOS)
    #include <dlfcn.h>
#elif defined(HE_PLATFORM_ANDROID)
    #include <android/native_activity.h>
    #include <dlfcn.h>
#endif
