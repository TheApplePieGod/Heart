#include "htpch.h"
#include "Input.h"

#include "Heart/Core/App.h"
#include "glfw/glfw3.h"

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
}