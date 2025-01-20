#include "hepch.h"
#include "MenuBar.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Project.h"
#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Core/Window.h"
#include "Heart/Scene/Entity.h"
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
        bool isRuntime = Editor::GetSceneState() != SceneState::Editing;
        if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up, height, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    Project* activeProject = Editor::GetState().ActiveProject.get();

                    bool scriptsCompiling = Editor::GetState().IsCompilingScripts;
                    bool scriptsInUse = isRuntime || scriptsCompiling;

                    if (activeProject)
                    {
                        if (scriptsInUse)
                            ImGui::BeginDisabled();
                        if (ImGui::MenuItem("Reload Client Scripts"))
                            activeProject->LoadScriptsPlugin();
                        if (ImGui::MenuItem("Build & Load Client Scripts", "Ctrl+B"))
                            Editor::StartScriptCompilation();
                        if (scriptsInUse)
                            ImGui::EndDisabled();
                        ImGui::Separator();
                    }

                    if (activeProject && ImGui::MenuItem("Save Project", "Ctrl+S"))
                        Editor::SaveProject();
                    
                    if (scriptsInUse)
                        ImGui::BeginDisabled();
                    if (activeProject && ImGui::BeginMenu("Export Project"))
                    {
                        ExportPlatform platform = ExportPlatform::None;
                        if (ImGui::MenuItem("Current Platform"))
                            platform = ExportPlatform::CurrentPlatform;
                        if (ImGui::MenuItem("Windows"))
                            platform = ExportPlatform::Windows;
                        if (ImGui::MenuItem("MacOS"))
                            platform = ExportPlatform::MacOS;
                        if (ImGui::MenuItem("Android"))
                            platform = ExportPlatform::Android;

                        if (platform != ExportPlatform::None)
                        {
                            Heart::HString8 path = Heart::FilesystemUtils::OpenFolderDialog(activeProject->GetPath(), "Export Parent Directory");
                            if (!path.IsEmpty())
                                activeProject->Export(path, platform);
                        }

                        ImGui::EndMenu();
                    }
                    if (scriptsInUse)
                        ImGui::EndDisabled();

                    if (scriptsCompiling)
                        ImGui::BeginDisabled();
                    if (ImGui::MenuItem("New Project"))
                        newProjectModalOpened = true;

                    if (ImGui::MenuItem("Open Project"))
                    {
                        Heart::HString8 path = Heart::FilesystemUtils::OpenFileDialog(Heart::AssetManager::GetAssetsDirectory(), "Open Project", "*.heproj");
                        if (!path.IsEmpty())
                            Project::LoadFromPath(path);
                    }
                    if (scriptsCompiling)
                        ImGui::EndDisabled();

                    ImGui::Separator();

                    if (scriptsInUse)
                        ImGui::BeginDisabled();
                    if (Editor::GetEditorSceneAsset() && ImGui::MenuItem("Save Scene", "Ctrl+S"))
                        Editor::SaveScene();
                        
                    if (ImGui::MenuItem("New Scene"))
                        Editor::ClearScene();

                    if (ImGui::MenuItem("Save Scene As"))
                    {
                        Heart::HString8 path = Heart::FilesystemUtils::SaveAsDialog(
                            Heart::AssetManager::GetAssetsDirectory(),
                            "Save Scene As",
                            "Scene",
                            "hescn",
                            "hescn"
                        );
                        if (!path.IsEmpty())
                        {
                            Heart::SceneAsset::SerializeScene(path, &Editor::GetEditorScene());
                            Editor::SetEditorSceneAsset(
                                Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, path)
                            );
                        }
                    }

                    if (ImGui::MenuItem("Load Scene"))
                    {
                        Heart::HString8 path = Heart::FilesystemUtils::OpenFileDialog(Heart::AssetManager::GetAssetsDirectory(), "Load Scene", "*.hescn");
                        if (!path.IsEmpty())
                        {
                            Heart::UUID assetId = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Scene, path);
                            Editor::OpenSceneFromAsset(assetId);
                        }
                    }
                    if (scriptsInUse)
                        ImGui::EndDisabled();

                    ImGui::Separator();

                    if (ImGui::MenuItem("Toggle Fullscreen", "F11"))
                        EditorApp::Get().GetWindow().ToggleFullscreen();

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
                        ImGui::MenuItem(pair.second->GetName().Data(), nullptr, &open);
                        pair.second->SetOpen(open);
                    }

                    ImGui::MenuItem("ImGui Demo", nullptr, &Editor::s_ImGuiDemoOpen);

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        if (RenderNewProjectDialog(newProjectModalOpened, m_NewProjectPath, m_NewProjectName))
            Project::CreateAndLoad(m_NewProjectPath, m_NewProjectName);

        ImGui::PopStyleVar(2);
    }

    bool MenuBar::RenderNewProjectDialog(
        bool openDialog,
        Heart::HString8& newPath,
        Heart::HString8& newName)
    {
        bool created = false;

        if (openDialog)
            ImGui::OpenPopup("New Project");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("New Project", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Path (new folder will be created inside):");
            ImGui::BeginDisabled();
            ImGui::InputText("##ProjPath", (char*)newPath.Data(), newPath.Count(), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("...##ProjPathSelect"))
                newPath = Heart::FilesystemUtils::OpenFolderDialog("", "Select Project Parent Directory");

            ImGui::Spacing();
            ImGui::Text("Name");
            Heart::ImGuiUtils::InputText("##ProjName", newName);

            ImGui::Spacing();
            ImGui::Separator();

            bool disabled = newPath.IsEmpty() || newName.IsEmpty();
            if (disabled) ImGui::BeginDisabled();
            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                created = true;
                ImGui::CloseCurrentPopup();
            }
            if (disabled) ImGui::EndDisabled();

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        return created;
    }
}
