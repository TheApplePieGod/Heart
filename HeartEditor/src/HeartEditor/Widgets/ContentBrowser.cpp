#include "htpch.h"
#include "ContentBrowser.h"

#include "HeartEditor/EditorApp.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace HeartEditor
{
namespace Widgets
{
    ContentBrowser::ContentBrowser()
    {
        ScanDirectory();
    }

    void ContentBrowser::InitializeTextureReistry()
    {
        m_CBTextures = Heart::CreateScope<Heart::TextureRegistry>();
        m_CBTextures->RegisterTexture("folder", "assets/textures/folder.png");
        m_CBTextures->RegisterTexture("file", "assets/textures/file.png");
    }

    void ContentBrowser::OnImGuiRender()
    {
        if (m_ShouldRescan)
            ScanDirectory();

        ImGui::Text("test");

        f32 windowWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
        f32 windowHeight = ImGui::GetContentRegionAvail().y;

        // update the 'max size'
        m_WindowSizes.y = windowWidth - m_WindowSizes.x - 13.f;

        // draw the splitter that allows for child window resizing
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = ImVec2(window->DC.CursorPos.x + m_WindowSizes.x, window->DC.CursorPos.y);
        bb.Max = ImGui::CalcItemSize(ImVec2(3.f, windowHeight), 0.0f, 0.0f);
        bb.Max = { bb.Max.x + bb.Min.x, bb.Max.y + bb.Min.y };
        ImGui::SplitterBehavior(bb, id, ImGuiAxis_X, &m_WindowSizes.x, &m_WindowSizes.y, 100.f, 100.f, 0.0f);

        // tree
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::BeginChild("cbtree", ImVec2(m_WindowSizes.x, windowHeight), false);

        ImGui::EndChild();

        ImGui::SameLine(0.f, 10.f);

        // file list (jank)
        ImGui::BeginChild("cbfiles", ImVec2(m_WindowSizes.y, windowHeight), false);
        f32 currentExtent = 999999.f; // to prevent SameLine spacing in first row
        for (const auto& entry : m_DirectoryList)
        {
            if (currentExtent + m_CardSize.x + m_CardSpacing >= m_WindowSizes.y - 50.f)
                currentExtent = 0.f;
            else
            {
                ImGui::SameLine(0.f, m_CardSpacing);
                currentExtent += m_CardSpacing;
            }

            RenderFileCard(entry);
            currentExtent += m_CardSize.x;
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void ContentBrowser::ScanDirectory()
    {
        m_DirectoryList.clear();
        for (const auto& entry : std::filesystem::directory_iterator(m_DefaultAssetDirectory))
            m_DirectoryList.push_back(entry);
        m_ShouldRescan = false;
    }

    void ContentBrowser::RenderFileCard(const std::filesystem::directory_entry& entry)
    {
        ImGui::BeginGroup();
        auto entryName = entry.path().filename().generic_u8string();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::ImageButton(
            m_CBTextures->LoadTexture(entry.is_directory() ? "folder" : "file")->GetImGuiHandle(),
            { m_CardSize.x, m_CardSize.y }
        );
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            m_DefaultAssetDirectory = std::filesystem::path(m_DefaultAssetDirectory).append(entryName).generic_u8string();
            m_ShouldRescan = true;
        }
        ImGui::PopStyleColor();

        // center and wrap the filename
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetItemRectSize().x);
        ImGui::TextWrapped(entryName.c_str());
        ImGui::EndGroup();
        ImGui::PopTextWrapPos();
    }
}
}