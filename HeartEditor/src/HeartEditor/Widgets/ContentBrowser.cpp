#include "hepch.h"
#include "ContentBrowser.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/SceneAsset.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Heart/Util/ImGuiUtils.h"

#include "HeartEditor/Widgets/MaterialEditor.h"

namespace HeartEditor
{
namespace Widgets
{
    ContentBrowser::ContentBrowser(const Heart::HStringView8& name, bool initialOpen)
            : Widget(name, initialOpen)
    {
        ScanDirectory();
    }

    void ContentBrowser::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);
        
        if (m_ShouldRescan)
            ScanDirectory();

        Heart::ImGuiUtils::ResizableWindowSplitter(
            m_WindowSizes,
            { 100.f, 100.f },
            true,
            3.f,
            10.f,
            false,
            [&]() { RenderDirectoryNode(m_DirectoryStack[0]); },
            [&]() { RenderFileList(); }
        );

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ContentBrowser::ScanDirectory()
    {
        if (Heart::AssetManager::GetAssetsDirectory().IsEmpty())
            return;

        bool isRoot = m_DirectoryStack[m_DirectoryStackIndex].IsEmpty();

        // Populate the directory list with each item in the directory
        m_DirectoryList.Clear();
        try
        {
            m_ShouldRescan = false;
            for (const auto& entry : std::filesystem::directory_iterator(
                std::filesystem::path(Heart::AssetManager::GetAssetsDirectory().Data())
                    .append(m_DirectoryStack[m_DirectoryStackIndex].Data()))
            )
            {
                // Root (exclude unnecessary folders)
                Heart::HString filename = entry.path().filename().generic_u8string();
                if (isRoot && (
                    filename == ".vs" ||
                    filename == "bin" ||
                    filename == "obj"
                ))
                    continue;
                m_DirectoryList.Add(entry);
            }
        }
        catch (std::filesystem::filesystem_error e) // likely failed to open directory so just reset
        {
            HE_ENGINE_LOG_ERROR("Failed to scan directory: {0}", e.what());
            m_DirectoryStack.Resize(1);
            m_DirectoryStackIndex = 0;
        }  
    }

    void ContentBrowser::PushDirectoryStack(const Heart::HStringView8& entry)
    {
        m_DirectoryStack.Add(entry);
        m_DirectoryStackIndex++;
        m_ShouldRescan = true;
    }

    void ContentBrowser::RenderDirectoryNode(const Heart::HStringView8& path, u32 depth)
    {
        auto absolutePath = std::filesystem::path(Heart::AssetManager::GetAssetsDirectory().Data()).append(path.Data());

        // Exclude unnecessary folders from list
        Heart::HString8 filename = absolutePath.filename().generic_u8string();
        if (depth == 1 && (
            filename == ".vs" ||
            filename == "bin" ||
            filename == "obj"
        ))
            return;

        // Get each item in the directory
        Heart::HVector<std::filesystem::directory_entry> directories;
        try
        {
            if (!absolutePath.empty())
                for (const auto& entry : std::filesystem::directory_iterator(absolutePath))
                    if (entry.is_directory())
                        directories.Add(entry);
        }
        catch (std::exception e) // likely invalid path so cut off this node
        { return; }

        // Render the directory tree node
        bool selected = path == m_DirectoryStack[m_DirectoryStackIndex];
        ImGuiTreeNodeFlags node_flags = (directories.Count() > 0 ? 0 : ImGuiTreeNodeFlags_Leaf) | ImGuiTreeNodeFlags_OpenOnArrow | (selected ? ImGuiTreeNodeFlags_Selected : 0);
        bool open = ImGui::TreeNodeEx(path.IsEmpty() ? "Project Root" : path.Data(), node_flags, path.IsEmpty() ? "Project Root" : filename.Data());
        if (!selected && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            PushDirectoryStack(path);
        
        // Create the drop target
        FileTransferDropTarget(absolutePath);

        // If open, recursively render the subdirectories
        if (open)
        {
            for (auto& entry : directories)
                RenderDirectoryNode(entry.path().generic_u8string(), depth + 1);
            ImGui::TreePop();
        }
    }

    void ContentBrowser::RenderFileList()
    {
        if (Heart::AssetManager::GetAssetsDirectory().IsEmpty())
            return;

        // Go backwards in the directory stack
        if (ImGui::Button("<##back"))
        {
            if (--m_DirectoryStackIndex < 0)
                m_DirectoryStackIndex++;
            else
                m_ShouldRescan = true;
        }

        ImGui::SameLine(0.f, 5.f);

        // Go forwards in the directory stack
        if (ImGui::Button(">##forwards"))
        {
            if (++m_DirectoryStackIndex >= m_DirectoryStack.Count())
                m_DirectoryStackIndex--;
            else
                m_ShouldRescan = true;
        }

        ImGui::SameLine(0.f, 10.f);

        // Rescan the current directory
        if (ImGui::Button("Refresh"))
            m_ShouldRescan = true;

        ImGui::SameLine(0.f, 10.f);

        if (ImGui::Button("Create"))
            ImGui::OpenPopup("CreatePopup");
        // Create asset popup
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        if (ImGui::BeginPopup("CreatePopup"))
        {
            if (ImGui::MenuItem("New Folder"))
            {
                std::filesystem::path path = Heart::AssetManager::GetAssetsDirectory().Data();
                path.append(m_DirectoryStack[m_DirectoryStackIndex].Data());
                Heart::HString8 fileName = "NewFolder";
                fileName += std::to_string(Heart::UUID());
                std::filesystem::create_directory(path.append(fileName.Data()));
                m_ShouldRescan = true;
            }
            if (ImGui::MenuItem("New Material"))
            {
                std::filesystem::path path = Heart::AssetManager::GetAssetsDirectory().Data();
                path.append(m_DirectoryStack[m_DirectoryStackIndex].Data());
                Heart::HString8 fileName = "NewMaterial";
                fileName += std::to_string(Heart::UUID());
                fileName += ".hemat";
                Heart::Material defaultMaterial;
                Heart::MaterialAsset::SerializeMaterial(path.append(fileName.Data()).generic_u8string(), defaultMaterial);
                m_ShouldRescan = true;
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        // Size-aware file list (TODO: redo this)
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

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1))
            ImGui::OpenPopup("CreatePopup");
    }

    void ContentBrowser::RenderFileCard(const std::filesystem::directory_entry& entry)
    {
        Heart::HString8 entryName = entry.path().filename().generic_u8string();
        Heart::HString8 fullPath = entry.path().generic_u8string();
        Heart::HString8 relativePath = Heart::AssetManager::GetRelativePath(fullPath);

        ImGui::BeginGroup();
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
        
        // Render the icon based on the file
        ImGui::PushID(entryName.Data());
        ImGui::ImageButton(
            Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(entry.is_directory() ? "editor/folder.png" :  "editor/file.png", true)->GetTexture()->GetImGuiHandle(),
            { m_CardSize.x, m_CardSize.y }
        );
        ImGui::PopID();

        // File transfer drag source
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("FileTransfer", fullPath.Data(), fullPath.Count() * sizeof(char) + 1);
            ImGui::Image(
                Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(entry.is_directory() ? "editor/folder.png" :  "editor/file.png", true)->GetTexture()->GetImGuiHandle(),
                { m_CardSize.x * 0.5f, m_CardSize.y * 0.5f }
            );
            ImGui::EndDragDropSource();
        }

        // Create the drop target
        FileTransferDropTarget(entry);

        // If the card is clicked
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            if (entry.is_directory())
            {
                // Make the current location the front of the stack
                m_DirectoryStack.Resize(m_DirectoryStackIndex + 1);

                // Push new directory to the stack
                PushDirectoryStack(
                    std::filesystem::path(m_DirectoryStack[m_DirectoryStackIndex].Data())
                    .append(entryName.Data())
                    .generic_u8string()
                );
            }
            else
            {
                auto assetType = Heart::AssetManager::DeduceAssetTypeFromFile(entryName);

                // Attempt to open the file based on asset type
                if (assetType == Heart::Asset::Type::Material)
                    ((Widgets::MaterialEditor&)Editor::GetWindow("Material Editor")).SetSelectedMaterial(Heart::AssetManager::RegisterAsset(assetType, relativePath));
                else if (assetType == Heart::Asset::Type::Scene)
                {
                    Heart::UUID assetId = Heart::AssetManager::RegisterAsset(assetType, relativePath);
                    Editor::OpenSceneFromAsset(assetId);
                }
            }
        }

        ImGui::PopStyleColor();

        // Card right click popup
        bool deleteDialogOpen = false;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5.f, 5.f });
        if (ImGui::BeginPopupContextItem(entryName.Data()))
        {
            // Delete dialog
            if (ImGui::MenuItem("Delete"))
            {
                m_DeletingPath = entry;
                deleteDialogOpen = true;
            }

            // Enable rename mode
            if (ImGui::MenuItem("Rename"))
            {
                m_RenamingPath = entry;
                m_Rename = entryName;
                m_ShouldRename = true;
            }

            auto assetType = Heart::AssetManager::DeduceAssetTypeFromFile(entryName);
            if (assetType == Heart::Asset::Type::Material && ImGui::MenuItem("Open in Editor"))
                ((Widgets::MaterialEditor&)Editor::GetWindow("Material Editor")).SetSelectedMaterial(Heart::AssetManager::RegisterAsset(assetType, relativePath));

            ImGui::EndPopup();
        }

        auto deletePopupId = Heart::HStringView8("Delete##") + Heart::HStringView8(entryName);
        if (deleteDialogOpen)
            ImGui::OpenPopup(deletePopupId.Data());
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(deletePopupId.Data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure? '%s' will be deleted permanently.", entry.path().filename().generic_u8string().c_str());

            if (ImGui::Button("Delete", ImVec2(120, 0)))
            {
                std::filesystem::remove_all(entry);
                m_ShouldRescan = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        // Center and wrap the filename if we are not renaming
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetItemRectSize().x);
        if (m_RenamingPath.empty() || m_RenamingPath != entry)
            ImGui::TextWrapped(entryName.Data());
        else
        {
            // Resize the text input box 
            ImGui::SetNextItemWidth(ImGui::GetItemRectSize().x);

            // Render the input box
            Heart::HStringView8 id = "##Rename";
            Heart::ImGuiUtils::InputText((id + Heart::HStringView8(entryName)).Data(), m_Rename);
            if (m_ShouldRename)
                ImGui::SetKeyboardFocusHere(-1);

            // If we click off of the input box (TODO: ImGui::IsItemDeactivatedAfterEdit)
            if (!ImGui::IsItemHovered() && ImGui::IsAnyMouseDown())
            {
                // If we left click and the new name is not blank, attempt to rename the file
                if (ImGui::IsMouseDown(0) && !m_Rename.IsEmpty())
                {
                    try
                    {
                        auto newPath = m_RenamingPath.parent_path().append(m_Rename.Data());
                        std::filesystem::rename(m_RenamingPath, newPath);
                        Heart::AssetManager::RenameAsset(
                            m_RenamingPath.lexically_relative(Heart::AssetManager::GetAssetsDirectory().Data()).generic_u8string(),
                            newPath.lexically_relative(Heart::AssetManager::GetAssetsDirectory().Data()).generic_u8string()
                        );
                    }
                    catch (std::exception e) {}
                }
                
                // Otherwise just clear the new name
                m_RenamingPath.clear();
                m_ShouldRescan = true;
            }

            m_ShouldRename = false;
        }

        ImGui::EndGroup();
        ImGui::PopTextWrapPos();
    }

    void ContentBrowser::FileTransferDropTarget(const std::filesystem::path& destination)
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileTransfer"))
            {
                const char* payloadData = (const char*)payload->Data;
                std::filesystem::directory_entry src(payloadData);
                std::filesystem::path dst = destination;
                if (src.is_directory())
                    dst.append(src.path().filename().generic_u8string());

                try
                {
                    std::filesystem::copy(src, dst);
                    std::filesystem::remove_all(payloadData);
                    m_ShouldRescan = true;
                }
                catch (std::exception e) {}
            }
            ImGui::EndDragDropTarget();
        }
    }
}
}