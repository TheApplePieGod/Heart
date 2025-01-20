#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "Heart/Container/HVector.hpp"
#include "glm/vec2.hpp"

namespace HeartEditor
{
namespace Widgets
{
    class ContentBrowser : public Widget
    {
    public:
        ContentBrowser(const Heart::HStringView8& name, bool initialOpen);

        void OnImGuiRenderPostSceneUpdate() override;
        
        void Reset();
        
        inline void RefreshList() { m_ShouldRescan = true; }
        
    private:
        void ScanDirectory();
        void RenderFileCard(const std::filesystem::directory_entry& entry);
        void RenderDirectoryNode(const Heart::HString8& path, u32 depth = 0);
        void RenderFileList();
        void PushDirectoryStack(const Heart::HString8& entry);
        void FileTransferDropTarget(const std::filesystem::path& destination);

    private:
        const glm::vec2 m_CardSize = { 75.f, 75.f };
        const f32 m_CardSpacing = 5.f;
        glm::vec2 m_WindowSizes = { 150.f, 850.f };
        Heart::HVector<Heart::HString8> m_DirectoryStack = { "" };
        int m_DirectoryStackIndex = 0;
        Heart::HVector<std::filesystem::directory_entry> m_DirectoryList;
        bool m_ShouldRescan = false;
        bool m_CardHovered = false;
        bool m_Hovered = false;

        std::filesystem::path m_RenamingPath;
        std::filesystem::path m_DeletingPath;
        Heart::HString8 m_Rename = "";
        bool m_ShouldRename = false;
    };
}
}
