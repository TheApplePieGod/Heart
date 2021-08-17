#pragma once

#include "Heart/Core/PlatformDetection.h"

#ifdef HT_PLATFORM_WINDOWS
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

#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Heart/Core/Base.h"
#include "Heart/Core/Log.h"
#include "Heart/Core/Assert.h"

#ifdef HT_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
