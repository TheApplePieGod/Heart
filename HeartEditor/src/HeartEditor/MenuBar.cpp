#include "hepch.h"
#include "MenuBar.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Project.h"
#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Core/Window.h"
#include "Heart/Scene/Entity.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/SceneAsset.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/ImGuiUtils.h"
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
        
        bool newProjectModalOpened = false;
        if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (Project::GetActiveProject())
                    {
                        bool disabled = Editor::GetSceneState() != SceneState::Editing;
                        if (disabled)
                            ImGui::BeginDisabled();
                        if (ImGui::MenuItem("Reload Client Scripts"))
                            Project::GetActiveProject()->LoadScriptsPlugin();
                        if (disabled)
                            ImGui::EndDisabled();
                        ImGui::Separator();
                    }

                    if (Project::GetActiveProject() && ImGui::MenuItem("Save Project"))
                        Project::GetActiveProject()->SaveToDisk();

                    if (ImGui::MenuItem("New Project"))
                        newProjectModalOpened = true;

                    if (ImGui::MenuItem("Open Project"))
                    {
                        std::string path = Heart::FilesystemUtils::OpenFileDialog(Heart::AssetManager::GetAssetsDirectory(), "Open Project", "heproj");
                        if (!path.empty())
                            Project::LoadFromPath(path);
                    }

                    ImGui::Separator();

                    if (Editor::GetEditorSceneAsset() && ImGui::MenuItem("Save Scene"))
                    {
                        auto asset = Heart::AssetManager::RetrieveAsset<Heart::SceneAsset>(Editor::GetEditorSceneAsset());
                        if (asset && asset->IsValid())
                            asset->Save(&Editor::GetEditorScene());
                    }

                    if (ImGui::MenuItem("New Scene"))
                        Editor::ClearScene();

                    if (ImGui::MenuItem("Save Scene As"))
                    {
                        std::string path = Heart::FilesystemUtils::SaveAsDialog(Heart::AssetManager::GetAssetsDirectory(), "Save Scene As", "Scene", "hescn");
                        if (!path.empty())
                            Heart::SceneAsset::SerializeScene(path, &Editor::GetEditorScene());
                    }

                    if (ImGui::MenuItem("Load Scene"))
                    {
                        std::string path = Heart::FilesystemUtils::OpenFileDialog(Heart::AssetManager::GetAssetsDirectory(), "Load Scene", "hescn");
                        if (!path.empty())
                        {
                            Heart::UUID assetId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, path);
                            Editor::OpenSceneFromAsset(assetId);
                        }
                    }

                    ImGui::Separator();

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
                    for (auto& pair : Editor::s_Windows)
                    {
                        bool open = pair.second->IsOpen();
                        ImGui::MenuItem(pair.second->GetName().c_str(), nullptr, &open);
                        pair.second->SetOpen(open);
                    }

                    ImGui::MenuItem("ImGui Demo", nullptr, &Editor::s_ImGuiDemoOpen);

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        if (newProjectModalOpened)
            ImGui::OpenPopup("New Project");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("New Project", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Path (new folder will be created inside):");
            ImGui::BeginDisabled();
            ImGui::InputText("##ProjPath", (char*)m_NewProjectPath.c_str(), m_NewProjectPath.size(), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("...##ProjPathSelect"))
                m_NewProjectPath = Heart::FilesystemUtils::OpenFolderDialog("", "Select Project Parent Directory");

            ImGui::Spacing();
            ImGui::Text("Name");
            Heart::ImGuiUtils::InputText("##ProjName", m_NewProjectName);

            ImGui::Spacing();
            ImGui::Separator();

            bool disabled = m_NewProjectPath.length() == 0 || m_NewProjectName.length() == 0;
            if (disabled) ImGui::BeginDisabled();
            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                Project::CreateAndLoad(m_NewProjectPath, m_NewProjectName);
                ImGui::CloseCurrentPopup();
            }
            if (disabled) ImGui::EndDisabled();

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar(2);
    }
}