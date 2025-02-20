#include "hepch.h"
#include "ProjectSettings.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Project.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
namespace Widgets
{
    void ProjectSettings::OnImGuiRenderPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        Project* activeProject = Editor::GetState().ActiveProject.get();
        if (!activeProject)
        {
            ImGui::Text("No project loaded");
        }
        else
        {
            ImGui::Text("Project Path:");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            ImGui::InputText(
                "##ProjPath",
                (char*)Heart::AssetManager::GetAssetsDirectory().Data(),
                Heart::AssetManager::GetAssetsDirectory().Count(),
                ImGuiInputTextFlags_ReadOnly
            );
            ImGui::EndDisabled();

            // TODO: implement project renaming
            ImGui::Text("Project Name:");
            ImGui::SameLine();
            ImGui::BeginDisabled();
            Heart::ImGuiUtils::InputText("##ProjName", activeProject->m_Name);
            ImGui::EndDisabled();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}
