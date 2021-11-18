#include "htpch.h"
#include "ContentBrowser.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
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

    void ContentBrowser::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();
        
        if (m_ShouldRescan)
            ScanDirectory();

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
        RenderDirectoryNode(m_DirectoryStack[0]);
        ImGui::EndChild();
        
        ImGui::SameLine(0.f, 10.f);

        ImGui::BeginChild("cbfiles", ImVec2(m_WindowSizes.y, windowHeight), false);
        // file list navigation
        if (ImGui::Button("<##back"))
        {
            if (--m_DirectoryStackIndex < 0)
                m_DirectoryStackIndex++;
            else
                m_ShouldRescan = true;
        }
        ImGui::SameLine(0.f, 5.f);
        if (ImGui::Button(">##forwards"))
        {
            if (++m_DirectoryStackIndex >= m_DirectoryStack.size())
                m_DirectoryStackIndex--;
            else
                m_ShouldRescan = true;
        }
        ImGui::SameLine(0.f, 10.f);
        if (ImGui::Button("Refresh"))
            m_ShouldRescan = true;

        // file list (jank)
        ImGui::BeginChild("cbfileslist", ImVec2(m_WindowSizes.y, ImGui::GetContentRegionAvail().y));
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

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void ContentBrowser::ScanDirectory()
    {
        m_DirectoryList.clear();
        try
        {
            m_ShouldRescan = false;
            for (const auto& entry : std::filesystem::directory_iterator(m_DirectoryStack[m_DirectoryStackIndex]))
                m_DirectoryList.push_back(entry);
        }
        catch (std::exception e) // likely failed to open directory so just reset
        {
            m_DirectoryStack.resize(1);
            m_DirectoryStackIndex = 0;
        }  
    }

    void ContentBrowser::PushDirectoryStack(const std::string& entry)
    {
        m_DirectoryStack.push_back(entry);
        m_DirectoryStackIndex++;
        m_ShouldRescan = true;
    }

    void ContentBrowser::RenderDirectoryNode(const std::string& path)
    {
        std::vector<std::filesystem::directory_entry> directories;
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(Heart::AssetManager::GetAssetsDirectory()).append(path)))
            if (entry.is_directory())
                directories.push_back(entry);

        bool selected = path == m_DirectoryStack[m_DirectoryStackIndex];
        ImGuiTreeNodeFlags node_flags = (directories.size() > 0 ? 0 : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (selected ? ImGuiTreeNodeFlags_Selected : 0);
        bool open = ImGui::TreeNodeEx(path.c_str(), node_flags, std::filesystem::path(path).filename().generic_u8string().c_str());
        if (!selected && ImGui::IsItemClicked())
            PushDirectoryStack(path.c_str());
        
        if (open)
        {
            for (auto& entry : directories)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
                RenderDirectoryNode(entry.path().generic_u8string().c_str());
                ImGui::PopStyleVar();
            }
            ImGui::TreePop();
        }
    }

    void ContentBrowser::RenderFileCard(const std::filesystem::directory_entry& entry)
    {
        ImGui::BeginGroup();
        auto entryName = entry.path().filename().generic_u8string();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
        std::string texturePath = "assets/textures/folder.png";
        if (!entry.is_directory())
        {
            if (Heart::AssetManager::IsAssetRegistered(Heart::Asset::Type::Texture, entry.path().generic_u8string()))
                texturePath = entry.path().generic_u8string();
            else
                texturePath = "assets/textures/file.png";
        }
        ImGui::ImageButton(
            Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(texturePath.c_str())->GetTexture()->GetImGuiHandle(),
            { m_CardSize.x, m_CardSize.y }
        );
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (entry.is_directory())
            {
                // make the current location the front of the stack
                m_DirectoryStack.resize(m_DirectoryStackIndex + 1);

                // push new directory to the stack
                PushDirectoryStack(std::filesystem::path(m_DirectoryStack[m_DirectoryStackIndex]).append(entryName).generic_u8string());
            }
            else
            {

            }
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