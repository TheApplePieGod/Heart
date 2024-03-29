#include "hepch.h"
#include "Input.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    bool Input::IsKeyPressed(KeyCode key)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(App::Get().GetWindow().GetWindowHandle());
		int state = glfwGetKey(window, static_cast<int>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
    }
    
    bool Input::IsMouseButtonPressed(MouseCode button)
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(App::Get().GetWindow().GetWindowHandle());
		int state = glfwGetMouseButton(window, static_cast<int>(button));
		return state == GLFW_PRESS;
    }
    
    glm::vec2 Input::GetScreenMousePos()
    {
        GLFWwindow* window = static_cast<GLFWwindow*>(App::Get().GetWindow().GetWindowHandle());
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		return { (f32)xPos, (f32)yPos };
    }

    void Input::SetMousePosition(double newX, double newY)
    {
        s_LastMousePosX = newX;
        s_LastMousePosY = newY;
    }

    void Input::UpdateMousePosition(double newX, double newY)
    {
        s_MouseDeltaX += newX - s_LastMousePosX;
        s_MouseDeltaY += newY - s_LastMousePosY;
        SetMousePosition(newX, newY);
    }

    void Input::UpdateScrollOffset(double newX, double newY)
    {
        s_ScrollOffsetX += newX;
        s_ScrollOffsetY += newY;
    }

    void Input::ClearDeltas()
    {
        s_MouseDeltaX = 0.0;
        s_MouseDeltaY = 0.0;
        s_ScrollOffsetX = 0.0;
        s_ScrollOffsetY = 0.0;
    }
}
