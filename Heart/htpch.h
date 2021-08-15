#pragma once

#ifdef HT_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Heart/Core/Base.h"

#ifdef HT_PLATFORM_WINDOWS
	#include <Windows.h>
#endif
