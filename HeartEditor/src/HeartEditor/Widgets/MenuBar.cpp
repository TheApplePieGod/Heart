#include "htpch.h"
#include "MenuBar.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    MenuBar::MenuBar()
    {
        // open default windows (todo: save/load)
        m_WindowStatuses["Content Browser"] = true;
        m_WindowStatuses["Properties Panel"] = true;
        m_WindowStatuses["Scene Hierarchy"] = true;
        m_WindowStatuses["Settings"] = true;
    }

    void MenuBar::OnImGuiRender()
    {   
        ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        
        if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Toggle Fullscreen", "F11"))
                        EditorApp::Get().GetWindow().ToggleFullscreen();

                    if (ImGui::BeginMenu("Switch Graphics API"))
                    {
                        for (u32 i = 1; i < HE_ARRAY_SIZE(Heart::RenderApi::TypeStrings); i++)
                        {
                            if (ImGui::MenuItem(Heart::RenderApi::TypeStrings[i]))
                                EditorApp::Get().SwitchGraphicsApi(static_cast<Heart::RenderApi::Type>(i));
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Quit", "Esc"))
                        EditorApp::Get().Close();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Windows"))
                {
                    for (auto& element : m_WindowStatuses)
                    {
                        ImGui::MenuItem(element.first.c_str(), nullptr, &element.second);
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        // if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        // {
        //     if (ImGui::BeginMenuBar())
        //     {
        //         if (ImGui::BeginMenu("Beep"))
        //         {
        //             if (ImGui::MenuItem("New")) {}

        //             ImGui::EndMenu();
        //         }

        //         ImGui::NewLine();
        //         ImGui::Button("Test button", ImVec2(200, 100));

        //         ImGui::EndMenuBar();
        //     }

        //     ImGui::End();
        // }

        ImGui::PopStyleVar(2);
    }
}
}