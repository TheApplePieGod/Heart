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
        static void UpdateMousePosition(double newX, double newY);

        inline static double GetMouseDeltaX() { return s_MouseDeltaX; }
        inline static double GetMouseDeltaY() { return s_MouseDeltaY; }
        inline static void ClearMouseDelta() { s_MouseDeltaX = 0.0; s_MouseDeltaY = 0.0; }

    private:
        static double s_MouseDeltaX;
        static double s_MouseDeltaY;
        static double s_LastMousePosX;
        static double s_LastMousePosY;
    };
}