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
        static void UpdateScrollOffset(double newX, double newY);

        inline static double GetMouseDeltaX() { return s_MouseDeltaX; }
        inline static double GetMouseDeltaY() { return s_MouseDeltaY; }
        inline static double GetScrollOffsetX() { return s_ScrollOffsetX; }
        inline static double GetScrollOffsetY() { return s_ScrollOffsetY; }
        static void ClearDeltas();

    private:
        static double s_MouseDeltaX;
        static double s_MouseDeltaY;
        static double s_ScrollOffsetX;
        static double s_ScrollOffsetY;
        static double s_LastMousePosX;
        static double s_LastMousePosY;
    };
}