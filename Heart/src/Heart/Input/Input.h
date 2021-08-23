#pragma once

#include "Heart/Input/KeyCodes.h"
#include "Heart/Input/MouseCodes.h"
#include "glm/vec2.hpp"

namespace Heart
{
    class Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetScreenMousePos();
    };
}