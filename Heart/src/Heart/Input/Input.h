#pragma once

#include "Heart/Input/KeyCodes.h"
#include "Heart/Input/MouseCodes.h"
#include "glm/vec2.hpp"

namespace Heart
{
    // TODO: switch to use MainWindow & threading stuff for GLFW
    class Input
    {
    public:
        /**
         * @brief Check if a keyboard key is pressed on the main window.
         *
         * @param key The KeyCode to check.
         */
        static bool IsKeyPressed(KeyCode key);

        /**
         * @brief Check if a mouse button is pressed on the main window.
         *
         * @param key The MouseCode to check.
         */
        static bool IsMouseButtonPressed(MouseCode button);

        /**
         * @brief Get the current coordinates of the mouse relative to the main window.
         *
         * @todo Use stored variables?
         *
         * @param key The MouseCode to check.
         * @returns The (x,y) coordinates with (0,0) being at the top left of the window.
         */
        static glm::vec2 GetScreenMousePos();

        // friend class window?
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