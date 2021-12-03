#include "hepch.h"
#include "Settings.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    void Settings::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.c_str(), &m_Open);

        ImGui::Text("Project Path:");
        ImGui::SameLine();
        ImGui::BeginDisabled();
        ImGui::InputText("##ProjPath", (char*)Heart::AssetManager::GetAssetsDirectory().c_str(), Heart::AssetManager::GetAssetsDirectory().size(), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("...##ProjPathSelect"))
        {
            std::string path = Heart::FilesystemUtils::OpenFolderDialog("", "Select Project Directory");
            if (!path.empty())
            {
                // Unload the scene
                Editor::SetActiveScene(Heart::CreateRef<Heart::Scene>());

                // Call switch on the app
                EditorApp::Get().SwitchAssetsDirectory(path);
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}