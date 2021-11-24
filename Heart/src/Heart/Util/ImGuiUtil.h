#pragma once

#include "glm/vec2.hpp"

namespace Heart
{
    struct ImGuiUtil
    {
        static void ResizableWindowSplitter(glm::vec2& storedWindowSizes, glm::vec2 minWindowSize, bool isHorizontal, float splitterThickness, float windowSpacing, std::function<void()> window1Contents, std::function<void()> window2Contents);
    };
}