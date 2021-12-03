#include "hepch.h"
#include "MenuBar.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Util/FilesystemUtils.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
    void MenuBar::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();
        
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
                    // if (ImGui::MenuItem("Save Scene"))
                    // {
                    //     std::string path = Heart::FilesystemUtils::SaveAsDialog(Heart::AssetManager::GetAssetsDirectory(), "Save Scene As", "Scene", "hescn");
                    //     if (!path.empty())
                    //         Heart::SceneAsset::SerializeScene(path, activeScene.get());
                    // }

                    if (ImGui::MenuItem("Save Scene As"))
                    {
                        std::string path = Heart::FilesystemUtils::SaveAsDialog(Heart::AssetManager::GetAssetsDirectory(), "Save Scene As", "Scene", "hescn");
                        if (!path.empty())
                            Heart::SceneAsset::SerializeScene(path, &Editor::GetActiveScene());
                    }

                    if (ImGui::MenuItem("Load Scene"))
                    {
                        std::string path = Heart::FilesystemUtils::OpenFileDialog(Heart::AssetManager::GetAssetsDirectory(), "Save Scene As", "hescn");
                        if (!path.empty())
                        {
                            Heart::UUID assetId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, path);
                            auto asset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(assetId);
                            if (asset && asset->IsValid())
                                Editor::SetActiveScene(asset->GetScene());
                        }
                    }

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
                    for (auto& window : Editor::s_Windows)
                    {
                        bool open = window.second->IsOpen();
                        ImGui::MenuItem(window.second->GetName().c_str(), nullptr, &open);
                        window.second->SetOpen(open);
                    }

                    ImGui::MenuItem("ImGui Demo", nullptr, &Editor::s_ImGuiDemoOpen);

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