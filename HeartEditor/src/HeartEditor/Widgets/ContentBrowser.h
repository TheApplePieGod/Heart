#pragma once

#include "HeartEditor/Widgets/Widget.h"
#include "glm/vec2.hpp"

namespace HeartEditor
{
namespace Widgets
{
    class ContentBrowser : public Widget
    {
    public:
        ContentBrowser(const Heart::HStringView8& name, bool initialOpen);

        void OnImGuiRender() override;

        inline void RefreshList() { m_ShouldRescan = true; }

    private:
        void ScanDirectory();
        void RenderFileCard(const std::filesystem::directory_entry& entry);
        void RenderDirectoryNode(const Heart::HStringView8& path, u32 depth = 0);
        void RenderFileList();
        void PushDirectoryStack(const Heart::HStringView8& entry);
        void FileTransferDropTarget(const std::filesystem::path& destination);

    private:
        const glm::vec2 m_CardSize = { 75.f, 75.f };
        const f32 m_CardSpacing = 5.f;
        glm::vec2 m_WindowSizes = { 0.f, 0.f };
        std::vector<Heart::HString8> m_DirectoryStack = { "" };
        int m_DirectoryStackIndex = 0;
        std::vector<std::filesystem::directory_entry> m_DirectoryList;
        bool m_ShouldRescan = false;

        std::filesystem::path m_RenamingPath;
        Heart::HString8 m_Rename = "";
        bool m_ShouldRename = false;
    };
}
}