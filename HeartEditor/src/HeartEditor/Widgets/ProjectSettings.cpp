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
    void ProjectSettings::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.DataUTF8(), &m_Open);

        Project* activeProject = Project::GetActiveProject();
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
                (char*)Heart::AssetManager::GetAssetsDirectory().DataUTF8(),
                Heart::AssetManager::GetAssetsDirectory().GetCountUTF8(),
                ImGuiInputTextFlags_ReadOnly
            );
            ImGui::EndDisabled();

            ImGui::Text("Project Name:");
            ImGui::SameLine();
            Heart::ImGuiUtils::InputText("##ProjName", activeProject->m_Name);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}
}