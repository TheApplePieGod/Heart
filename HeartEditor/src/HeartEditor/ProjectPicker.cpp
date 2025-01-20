#include "hepch.h"
#include "ProjectPicker.h"

#include "HeartEditor/EditorApp.h"
#include "HeartEditor/Editor.h"
#include "HeartEditor/Project.h"
#include "HeartEditor/MenuBar.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
    void ProjectPicker::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 8.f));
        ImGui::BeginChild("ProjectPicker", ImVec2(0.f, 0.f), ImGuiChildFlags_AlwaysUseWindowPadding);

        ImGui::PushFont(EditorApp::Get().GetImGuiInstance().GetLargeFont());
        ImGui::Text("Heart Editor");
        ImGui::PopFont();
        ImGui::Text("Select a project");

        ImGui::Dummy({ 0.f, 5.f });

        float height = ImGui::GetContentRegionMax().y;

        m_SelectedProject = ProjectDescriptor();
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(16.f, 32.f));
        ImGui::BeginChild("ScrollableTableRegion", ImVec2(-FLT_MIN, 300));
        if (ImGui::BeginTable("RecentList", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_SizingStretchProp))
        {
            for (u32 i = 0; i < Editor::GetConfig().RecentProjects.Count(); i++)
            {
                auto& project = Editor::GetConfig().RecentProjects[i];

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", project.Name.Data());
                ImGui::TextColored({ 0.7f, 0.7f, 0.7f, 1.f }, "%s", project.Path.Data());

                ImGui::PushID(project.Path.Data());

                ImGui::TableSetColumnIndex(1);
                if (ImGui::Button("Open", { -FLT_MIN, 32.f }))
                    m_SelectedProject = project;

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::Dummy({ 0.f, 5.f });

        bool newProject = ImGui::Button("New Project", { -FLT_MIN, 50.f });
        if (MenuBar::RenderNewProjectDialog(newProject, m_NewProjectPath, m_NewProjectName))
        {
            m_SelectedProject.New = true;
            m_SelectedProject.Path = m_NewProjectPath;
            m_SelectedProject.Name = m_NewProjectName;
        }

        if (ImGui::Button("Open Project", { -FLT_MIN, 50.f }))
        {
            Heart::HString8 path = Heart::FilesystemUtils::OpenFileDialog(
                Heart::AssetManager::GetAssetsDirectory(),
                "Open Project",
                "*.heproj"
            );
            if (!path.IsEmpty())
            {
                Heart::HString8 filename = std::filesystem::path(path.Data()).filename().generic_u8string();
                m_SelectedProject.Path = path;
                m_SelectedProject.Name = Heart::HString8(filename.Split(".")[0]);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
}
